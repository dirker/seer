#include "QMessageListWidget.h"
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeWidgetItem>
#include <QtWidgets/QApplication>
#include <QtCore/QTime>
#include <QtCore/QDebug>


QMessageListWidget::QMessageListWidget (QWidget* parent) : QWidget(parent) {

    // Construct the UI.
    setupUi(this);

    Qt::WindowFlags flags = windowFlags();

    flags |= Qt::WindowStaysOnTopHint;

    setWindowFlags(flags);

    // Setup the widgets.
    messageTreeWidget->setSortingEnabled(false);
    messageTreeWidget->resizeColumnToContents(0); // timestamp
    messageTreeWidget->resizeColumnToContents(1); // message type icon
    messageTreeWidget->resizeColumnToContents(2); // message
    messageTreeWidget->clear();

    // Get icons.
    _informationIcon = QIcon(":/seer/resources/RelaxLightIcons/data-information.svg");
    _warningIcon     = QIcon(":/seer/resources/RelaxLightIcons/data-warning.svg");
    _criticalIcon    = QIcon(":/seer/resources/RelaxLightIcons/data-error.svg");
    _questionIcon    = QIcon(":/seer/resources/RelaxLightIcons/dialog-question.svg");


    // Connect things.
    QObject::connect(okPushButton,      &QToolButton::clicked,              this,  &QMessageListWidget::handleOkButtonClicked);

    hide();
}

QMessageListWidget::~QMessageListWidget () {
}

void QMessageListWidget::addMessage (const QString& message, QMessageBox::Icon messageType) {

    show();
    raise();

    // Create an entry with our message.
    QTreeWidgetItem* item = new QTreeWidgetItem;
    item->setText(0, QTime::currentTime().toString(Qt::TextDate));

    switch (messageType) {
        case QMessageBox::NoIcon:
            item->setIcon(1, _noIcon);
            break;
        case QMessageBox::Information:
            item->setIcon(1, _informationIcon);
            break;
        case QMessageBox::Warning:
            item->setIcon(1, _warningIcon);
            break;
        case QMessageBox::Critical:
            item->setIcon(1, _criticalIcon);
            break;
        case QMessageBox::Question:
            item->setIcon(1, _questionIcon);
            break;
        default:
            item->setIcon(1, _noIcon);
            break;
    }

    item->setText(2, message);

    // Insert the entry (at the end).
    messageTreeWidget->addTopLevelItem(item);

    // Re-adjust column sizes.
    messageTreeWidget->resizeColumnToContents(0);
    messageTreeWidget->resizeColumnToContents(1);
    messageTreeWidget->resizeColumnToContents(2);

    // Scroll to the bottom.
    QTreeWidgetItem* lastItem = messageTreeWidget->topLevelItem(messageTreeWidget->topLevelItemCount()-1);
    if (lastItem) {
        messageTreeWidget->scrollToItem(lastItem);
        messageTreeWidget->clearSelection();
        lastItem->setSelected(true);
    }
}

void QMessageListWidget::handleOkButtonClicked () {

    hide();
}

