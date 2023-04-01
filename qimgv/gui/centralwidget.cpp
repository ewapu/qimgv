#include "centralwidget.h"

CentralWidget::CentralWidget(std::shared_ptr<DocumentWidget> _docWidget, std::shared_ptr<FolderViewProxy> _folderView, QWidget *parent)
    : QSplitter(parent),
      documentView(_docWidget),
      folderView(_folderView)
{
    setMouseTracking(true);
    if(!documentView || !folderView)
        qDebug() << "[CentralWidget] Error: child widget is null. We will crash now.  Bye.";

    if(folderView)
        addWidget(folderView.get());
    addWidget(documentView.get());

    auto updateStyle = [this]() {
        auto color = settings->colorScheme().folderview_hc.name();
        if (handle(1))
            handle(1)->setStyleSheet("background-color: " + color + ";");
    };
    connect(settings, &Settings::settingsChanged, updateStyle);
    updateStyle();

    setSizes(QList({100,100}));
    showDocumentView();
}

void CentralWidget::showDocumentView() {
    if(mode == MODE_DOCUMENT)
        return;
    mode = MODE_DOCUMENT;
    documentView->show();
    documentView->viewWidget()->startPlayback();
}

void CentralWidget::showFolderView() {
    if(mode == MODE_FOLDERVIEW)
        return;

    mode = MODE_FOLDERVIEW;
    documentView->hide();
    documentView->viewWidget()->stopPlayback();
}

void CentralWidget::toggleViewMode() {
    (mode == MODE_DOCUMENT) ? showFolderView() : showDocumentView();
}

ViewMode CentralWidget::currentViewMode() {
    return mode;
}
