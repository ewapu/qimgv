#include "centralwidget.h"

CentralWidget::CentralWidget(std::shared_ptr<DocumentWidget> _docWidget, std::shared_ptr<FolderViewProxy> _folderView, QWidget *parent)
    : QSplitter(parent),
      documentView(_docWidget),
      folderView(_folderView),
      mode(MODE_INIT),
      splitterState()
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

    setSizes(QList({0,0}));

    folderView->show();
    documentView->show();
}

void CentralWidget::setMode(ViewMode new_mode) {
    if (mode == new_mode)
        return;
    if (mode == MODE_SPLIT)
        splitterState = this->saveState();

    int val = 1000;
    switch (new_mode) {
    case MODE_DOCUMENT:
        setSizes({0,val});
        if (mode != MODE_DOCUMENT && mode != MODE_SPLIT)
            documentView->viewWidget()->startPlayback();
        documentView->setPanelEnabled(settings->panelEnabled());
        break;
    case MODE_FOLDERVIEW:
        documentView->viewWidget()->stopPlayback();
        setSizes({val,0});
        break;
    case MODE_SPLIT:
        if (splitterState.isEmpty())
            setSizes({val,val});
        else
            this->restoreState(splitterState);
        if (mode != MODE_DOCUMENT && mode != MODE_SPLIT)
            documentView->viewWidget()->startPlayback();

        documentView->setPanelEnabled(false);
        break;
    case MODE_INIT:
        assert(false);
    }
    mode = new_mode;
}

ViewMode CentralWidget::currentViewMode() {
    return mode;
}
