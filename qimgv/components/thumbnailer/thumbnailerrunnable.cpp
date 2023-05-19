#include "thumbnailerrunnable.h"

#include <memory>

ThumbnailerRunnable::ThumbnailerRunnable(ThumbnailCache* _cache, QString _path, int _size, bool _crop, bool _force) :
    path(_path),
    size(_size),
    crop(_crop),
    force(_force),
    cache(_cache)
{
}

void ThumbnailerRunnable::run() {
    emit taskStart(path, size);
    std::shared_ptr<Thumbnail> thumbnail = generate(cache, path, size, crop, force);
    emit taskEnd(thumbnail, path);
}

QString ThumbnailerRunnable::generateIdString(QString path, int size, bool crop) {
    QString queryStr = path + QString::number(size);
    if(crop)
        queryStr.append("s");
    queryStr = QString("%1").arg(QString(QCryptographicHash::hash(queryStr.toUtf8(),QCryptographicHash::Md5).toHex()));
    return queryStr;
}

std::shared_ptr<Thumbnail> ThumbnailerRunnable::generate(ThumbnailCache* cache, QString path, int size, bool crop, bool force) {
    DocumentInfo imgInfo(path);
    QString thumbnailId = generateIdString(path, size, crop);
    std::unique_ptr<QImage> image;

    QString time = QString::number(imgInfo.lastModified().toMSecsSinceEpoch());

    if(!force && cache) {
        image.reset(cache->readThumbnail(thumbnailId));
        if(image && image->text("lastModified") != time)
            image.reset(nullptr);
    }

    if(!image) {
        if(imgInfo.type() == DocumentType::NONE) {
            std::shared_ptr<Thumbnail> thumbnail(new Thumbnail(imgInfo.fileName(), "", size, nullptr));
            return thumbnail;
        }
        std::pair<QImage*, QSize> pair;
        if(imgInfo.type() == VIDEO)
            pair = createVideoThumbnail(path, size, crop);
        else
            pair = createThumbnail(imgInfo.filePath(), imgInfo.format().toStdString().c_str(), size, crop);

        if (!pair.first)
            return  std::make_shared<Thumbnail>(imgInfo.fileName(), "", size, nullptr);

        image.reset(pair.first);
        QSize originalSize = pair.second;

        image = ImageLib::exifRotated(std::move(image), imgInfo.exifOrientation());

        // put in image info
        image->setText("originalWidth", QString::number(originalSize.width()));
        image->setText("originalHeight", QString::number(originalSize.height()));
        image->setText("lastModified", time);

        if(imgInfo.type() == ANIMATED)
            image->setText("label", " [a]");
        else if(imgInfo.type() == VIDEO)
            image->setText("label", " [v]");

        if(cache) {
            // save thumbnail if it makes sense
            // FIXME: avoid too much i/o
            if(originalSize.width() > size || originalSize.height() > size || imgInfo.type() == VIDEO)
                cache->saveThumbnail(image.get(), thumbnailId);
        }
    }
    auto && tmpPixmap = new QPixmap(image->size());
    *tmpPixmap = QPixmap::fromImage(*image);
    tmpPixmap->setDevicePixelRatio(qApp->devicePixelRatio());

    QString label;
    if(tmpPixmap->width() == 0) {
        label = "error";
    } else  {
        // put info into Thumbnail object
        label = image->text("originalWidth") +
                "x" +
                image->text("originalHeight") +
                image->text("label");
    }
    std::shared_ptr<QPixmap> pixmapPtr(tmpPixmap);
    std::shared_ptr<Thumbnail> thumbnail(new Thumbnail(imgInfo.fileName(), label, size, pixmapPtr));
    return thumbnail;
}

ThumbnailerRunnable::~ThumbnailerRunnable() {
}

std::pair<QImage*, QSize> ThumbnailerRunnable::createThumbnail(QString path, const char *format, int size, bool squared) {
    QImageReader *reader = new QImageReader(path, format);
    Qt::AspectRatioMode ARMode = squared?
                (Qt::KeepAspectRatioByExpanding):(Qt::KeepAspectRatio);
    QImage *result = nullptr;
    QSize originalSize;
    bool indexed = (reader->imageFormat() == QImage::Format_Indexed8);
    bool manualResize = indexed || !reader->supportsOption(QImageIOHandler::Size);
    if(!manualResize) { // resize during read via QImageReader (faster)
        QSize scaledSize = reader->size().scaled(size, size, ARMode);
        reader->setScaledSize(scaledSize);
        if(squared) {
            QRect clip(0, 0, size, size);
            QRect scaledRect(QPoint(0,0), scaledSize);
            clip.moveCenter(scaledRect.center());
            reader->setScaledClipRect(clip);
        }
        originalSize = reader->size();
        result = new QImage();
        if(!reader->read(result)) {
            // If read() returns false there's no guarantee that size conversion worked properly.
            // So we fallback to manual.
            // Se far I've seen this happen only on some weird (corrupted?) jpeg saved from camera
            manualResize = true;
            delete result;
            result = nullptr;
            // Force reset reader because it is really finicky
            // and can fail on the second read attempt (yeah wtf)
            reader->setFileName("");
            delete reader;
            reader = new QImageReader(path, format);
        }
    }
    if(manualResize) { // manual resize & crop. slower but should just work
        QImage *fullSize = new QImage();
        reader->read(fullSize);
        if(indexed) {
            auto newFmt = QImage::Format_RGB32;
            if(fullSize->hasAlphaChannel())
                newFmt = QImage::Format_ARGB32;
            auto tmp = new QImage(fullSize->convertToFormat(newFmt));
            delete fullSize;
            fullSize = tmp;
        }
        originalSize = fullSize->size();
        QSize scaledSize = fullSize->size().scaled(size, size, ARMode);
        if(squared) {
            QRect clip(0, 0, size, size);
            QRect scaledRect(QPoint(0,0), scaledSize);
            clip.moveCenter(scaledRect.center());
            QImage scaled = QImage(fullSize->scaled(scaledSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            result = ImageLib::croppedRaw(&scaled, clip);
        } else {
            result = new QImage(fullSize->scaled(scaledSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        }
        delete fullSize;
    }
    // force reader to close file so it can be deleted later
    reader->setFileName("");
    delete reader;
    return std::make_pair(result, originalSize);
}

std::pair<QImage*, QSize> ThumbnailerRunnable::createVideoThumbnail(QString path, int size, bool squared) {
    QFileInfo fi(path);
    QString tmpFilePath = settings->tmpDir() + fi.fileName() + ".png";
    QString tmpFilePathEsc = tmpFilePath;
    tmpFilePathEsc.replace("%", "%%");

    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);
    QString vf = "--vf=scale=w=%size%:h=%size%:force_original_aspect_ratio=decrease:flags=fast_bilinear" +
                 QString(squared ? ",pad=w=%size%:h=%size%:x=-1:y=-1" : "") + QString(",format=rgb24");
    vf.replace("%size%", QString::number(size));
    process.start(settings->mpvBinary(),
                  QStringList()
                          << "--start=15%"
                          << "--frames=1"
                          << "--aid=no"
                          << "--sid=no"
                          << "--no-config"
                          << "--load-scripts=no"
                          << "--quiet"
                          << "--sws-fast=yes"
                          << "--hr-seek=no"
                          << "--hwdec=no"
                          << "--vd-lavc-fast"
                          << "--vd-lavc-software-fallback=1"
                          << "--vd-lavc-skiploopfilter=all"
                          << "--demuxer-readahead-secs=0"
                          << "--demuxer-max-bytes=128KiB"
                          << "--sws-scaler=fast-bilinear"
                          << vf
                          << "--of=image2" << "--ofopts=update=1"
                          << "--ovc=rawvideo"
                          << "--o=-"
                          << path
    );

    if (!process.waitForStarted()) {
        qDebug() << "Failed to start mpv process.";
        return std::make_pair(nullptr, *(new QSize()));
    }

    process.waitForFinished();
    QByteArray sout = process.readAllStandardOutput();
    QByteArray serr = process.readAllStandardError();

    QRegularExpression reO(R"(\(.*? (\d+)x(\d+)( .*?fps\))?)");
    QRegularExpression reN(R"(VO: \[lavc\] (\d+)x(\d+))");
    QRegularExpressionMatch matchO = reO.match(serr);
    QRegularExpressionMatch matchN = reN.match(serr);
    if (!matchO.hasMatch() || !matchN.hasMatch()) {
        qDebug() << "Didn’t find video sizes in stderr " << !matchN.hasMatch();
        qDebug() << serr;
        qDebug() << path;
        return std::make_pair(nullptr, *(new QSize()));
    }
    int Owidth = matchO.captured(1).toInt(), Oheight = matchO.captured(2).toInt();
    int Nwidth = matchN.captured(1).toInt(), Nheight = matchN.captured(2).toInt();

    auto* image = new QImage(reinterpret_cast<const uchar*>(sout.constData()), Nwidth, Nheight, Nwidth*3, QImage::Format_RGB888);

    process.close();
    return std::make_pair(image, QSize(Owidth, Oheight));
}
