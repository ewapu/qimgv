#include "videocontrols.h"
#include "ui_videocontrols.h"

VideoControls::VideoControls(FloatingWidgetContainer *parent) :
    OverlayWidget(parent),
    ui(new Ui::VideoControls)
{
    ui->setupUi(this);
//    this->setAttribute(Qt::WA_NoMousePropagation, true);
    setMouseTracking(true);

    hide();
    ui->pauseButton->setIconPath(":res/icons/common/buttons/videocontrols/play24.png");
    ui->pauseButton->setAction("pauseVideo");
    ui->prevFrameButton->setIconPath(":res/icons/common/buttons/videocontrols/skip-backwards24.png");
    ui->prevFrameButton->setAction("frameStepBack");
    ui->nextFrameButton->setIconPath(":res/icons/common/buttons/videocontrols/skip-forward24.png");
    ui->nextFrameButton->setAction("frameStep");
    ui->muteButton->setIconPath(":/res/icons/common/buttons/videocontrols/mute-on24.png");
    ui->muteButton->setAction("toggleMute");

    ui->volumeBar->hide();
    ui->volumeBar->setValue(settings->volume());

    lastPosition = -1;

    readSettings();
    connect(settings, &Settings::settingsChanged, this, &VideoControls::readSettings);

    connect(ui->seekBar, &VideoSlider::sliderMovedX, this, &VideoControls::seek);
    connect(ui->volumeBar, &VideoSlider::sliderMovedX, this, &VideoControls::setVolume);

    if(parent)
        setContainerSize(parent->size());
}

void VideoControls::readSettings() {
    if(settings->panelEnabled() && settings->panelPosition() == PanelPosition::PANEL_BOTTOM)
        setPosition(FloatingWidgetPosition::TOP);
    else
        setPosition(FloatingWidgetPosition::BOTTOM);
}

VideoControls::~VideoControls() {
    delete ui;
}

void VideoControls::setMode(PlaybackMode _mode) {
    mode = _mode;
    ui->muteButton->setVisible( (mode == PLAYBACK_VIDEO) );
}

QString VideoControls::formatTimeString(int duration) {
    QString durationStr;
    if(mode == PLAYBACK_VIDEO) {
        int _time = duration;
        int hours = _time / 3600;
        _time -= hours * 3600;
        int minutes = _time / 60;
        int seconds = _time - minutes * 60;
        durationStr = QString("%1").arg(minutes, 2, 10, QChar('0')) + ":" +
                      QString("%1").arg(seconds, 2, 10, QChar('0'));
        if(hours)
            durationStr.prepend(QString("%1").arg(hours, 2, 10, QChar('0')) + ":");
    } else {
        durationStr = QString::number(duration);
    }
    return durationStr;
}

void VideoControls::setPlaybackDuration(int duration) {
    ui->seekBar->setRange(0, duration - 1);
    durationString = formatTimeString(duration);
    ui->positionLabel->setText(durationString + " / " + durationString);
    recalculateGeometry();
    ui->positionLabel->setText(" / " + durationString);
}

void VideoControls::setPlaybackPosition(int position) {
    QString s = formatTimeString(position);
    ui->positionLabel->setText(s + " / " + durationString);
    ui->seekBar->blockSignals(true);
    ui->seekBar->setValue(position);
    ui->seekBar->blockSignals(false);
    lastPosition = position;
}

void VideoControls::onPlaybackPaused(bool mode) {
    if(mode)
        ui->pauseButton->setIconPath(":res/icons/common/buttons/videocontrols/play24.png");
    else
        ui->pauseButton->setIconPath(":res/icons/common/buttons/videocontrols/pause24.png");
}

void VideoControls::onVideoMuted(bool mode) {
    if(mode)
        ui->muteButton->setIconPath(":res/icons/common/buttons/videocontrols/mute-on24.png");
    else
        ui->muteButton->setIconPath(":res/icons/common/buttons/videocontrols/mute-off24.png");
}

void VideoControls::mouseMoveEvent(QMouseEvent *event) {
    QWidget::mouseMoveEvent(event);
    QRect r = ui->muteButton->frameGeometry();
    r.setWidth(r.width() + ui->volumeBar->width() + ui->muteSpacer->sizeHint().width() + ui->volumeSpacer->sizeHint().width() + 5);
    qDebug() << ui->volumeBar->width() << ui->muteSpacer->sizeHint().width() << ui->volumeSpacer->sizeHint().width();
    qDebug() << event->pos() << r;
    if (r.contains(event->pos()))
        ui->volumeBar->show();
    else
        ui->volumeBar->hide();
}

QSize VideoControls::sizeHint() const {
    // by default all widgets are at minimum size, but we want the seekbar to be as big as possible
    QSize s = QWidget::sizeHint();
    int w = s.width() + ui->seekBar->maximumWidth() - ui->seekBar->minimumWidth();
    s.setWidth(qMin(w, dynamic_cast<QWidget*>(this->parent())->width() - 5));
    return s;
}

void VideoControls::resizeEvent(QResizeEvent *event) {
//    qDebug() << minimumSize();
//    qDebug() << size();
//    std::vector<QWidget*> priority = {ui->prevFrameButton, ui->nextFrameButton, ui->muteButton, ui->pauseButton, ui->positionLabel, ui->seekBar};
//    QList<QWidget*> childWidgets = findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
//
//    int minW =  std::accumulate(childWidgets.begin(), childWidgets.end(), 0, [this](int acc, const QWidget* widget) {
//                                    return acc + (widget->isVisibleTo(this) ? widget->minimumWidth() : 0);
//                                });
//
//    qDebug() << minW;
//    for (QWidget* c : priority) {
//        if (!c->isVisibleTo(this))
//            continue;
//        if (width() >= minW) {
//            break;
//        } else {
//            c->hide();
//            minW -= c->width();
//        }
//    }
//
//    std::reverse(priority.begin(), priority.end());
//    int pw = dynamic_cast<QWidget*>(this->parent())->width();
//    for (QWidget* c : priority) {
//        if (c->isVisibleTo(this))
//            continue;
//        if (width() + c->width() + 5 >= pw) {
//            break;
//        } else {
//            c->show();
//            pw -= c->width();
//        }
//    }
//    update();
//    QWidget::resizeEvent(event);
}