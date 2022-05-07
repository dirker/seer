#include "SeerArrayVisualizerWidget.h"
#include "SeerUtl.h"
#include <QtCharts/QLineSeries>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QScatterSeries>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QToolTip>
#include <QtGui/QIntValidator>
#include <QtGui/QIcon>
#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>
#include <QtCore/QRegExp>
#include <QtCore/QSettings>
#include <QtCore/QDebug>

SeerArrayVisualizerWidget::SeerArrayVisualizerWidget (QWidget* parent) : QWidget(parent) {

    // Init variables.
    _xVariableId = Seer::createID(); // Create two id's for queries.
    _xMemoryId   = Seer::createID();

    _yVariableId = Seer::createID(); // Create two id's for queries.
    _yMemoryId   = Seer::createID();

    _series      = 0;

    // Set up UI.
    setupUi(this);

    // Setup the widgets
    setWindowIcon(QIcon(":/seer/resources/seer_64x64.png"));
    setWindowTitle("Seer Array Visualizer");

    xArrayLengthLineEdit->setValidator(new QIntValidator(1, 9999999, this));
    yArrayLengthLineEdit->setValidator(new QIntValidator(1, 9999999, this));
    xArrayOffsetLineEdit->setValidator(new QIntValidator(0, 9999999, this));
    yArrayOffsetLineEdit->setValidator(new QIntValidator(0, 9999999, this));
    xArrayStrideLineEdit->setValidator(new QIntValidator(1, 9999999, this));
    yArrayStrideLineEdit->setValidator(new QIntValidator(1, 9999999, this));


    lineRadioButton->setChecked(true);

    xArrayDisplayFormatComboBox->setCurrentIndex(0);
    yArrayDisplayFormatComboBox->setCurrentIndex(0);

    handlexArrayDisplayFormatComboBox(0);
    handleyArrayDisplayFormatComboBox(0);

    // A single series chart.
    QChart* chart = new QChart;
    chart->legend()->hide();
    chart->createDefaultAxes();
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    arrayChartView->setRenderHint(QPainter::Antialiasing);
    arrayChartView->setChart(chart);

    // Connect things.
    QObject::connect(xRefreshToolButton,            &QToolButton::clicked,                                     this,  &SeerArrayVisualizerWidget::handlexRefreshButton);
    QObject::connect(yRefreshToolButton,            &QToolButton::clicked,                                     this,  &SeerArrayVisualizerWidget::handleyRefreshButton);
    QObject::connect(xArrayLengthLineEdit,          &QLineEdit::returnPressed,                                 this,  &SeerArrayVisualizerWidget::handlexRefreshButton);
    QObject::connect(yArrayLengthLineEdit,          &QLineEdit::returnPressed,                                 this,  &SeerArrayVisualizerWidget::handleyRefreshButton);
    QObject::connect(xArrayOffsetLineEdit,          &QLineEdit::returnPressed,                                 this,  &SeerArrayVisualizerWidget::handlexRefreshButton);
    QObject::connect(yArrayOffsetLineEdit,          &QLineEdit::returnPressed,                                 this,  &SeerArrayVisualizerWidget::handleyRefreshButton);
    QObject::connect(xArrayStrideLineEdit,          &QLineEdit::returnPressed,                                 this,  &SeerArrayVisualizerWidget::handlexRefreshButton);
    QObject::connect(yArrayStrideLineEdit,          &QLineEdit::returnPressed,                                 this,  &SeerArrayVisualizerWidget::handleyRefreshButton);
    QObject::connect(xVariableNameLineEdit,         &QLineEdit::returnPressed,                                 this,  &SeerArrayVisualizerWidget::handlexVariableNameLineEdit);
    QObject::connect(yVariableNameLineEdit,         &QLineEdit::returnPressed,                                 this,  &SeerArrayVisualizerWidget::handleyVariableNameLineEdit);
    QObject::connect(xArrayDisplayFormatComboBox,   QOverload<int>::of(&QComboBox::currentIndexChanged),       this,  &SeerArrayVisualizerWidget::handlexArrayDisplayFormatComboBox);
    QObject::connect(yArrayDisplayFormatComboBox,   QOverload<int>::of(&QComboBox::currentIndexChanged),       this,  &SeerArrayVisualizerWidget::handleyArrayDisplayFormatComboBox);

    QObject::connect(arrayTableWidget,              &SeerArrayWidget::dataChanged,                             this,  &SeerArrayVisualizerWidget::handleDataChanged);
    QObject::connect(splitter,                      &QSplitter::splitterMoved,                                 this,  &SeerArrayVisualizerWidget::handleSplitterMoved);
    QObject::connect(titleLineEdit,                 &QLineEdit::returnPressed,                                 this,  &SeerArrayVisualizerWidget::handleTitleLineEdit);
    QObject::connect(pointsCheckBox,                &QCheckBox::clicked,                                       this,  &SeerArrayVisualizerWidget::handlePointsCheckBox);
    QObject::connect(labelsCheckBox,                &QCheckBox::clicked,                                       this,  &SeerArrayVisualizerWidget::handleLabelsCheckBox);
    QObject::connect(lineTypeButtonGroup,           QOverload<int>::of(&QButtonGroup::idClicked),              this,  &SeerArrayVisualizerWidget::handleLineTypeButtonGroup);

    // Restore window settings.
    readSettings();
}

SeerArrayVisualizerWidget::~SeerArrayVisualizerWidget () {
}

void SeerArrayVisualizerWidget::setXVariableName (const QString& name) {

    setWindowTitle("Seer Array Visualizer - '" + name + "'");

    xVariableNameLineEdit->setText(name);
    setXVariableAddress("");

    if (xVariableNameLineEdit->text() == "") {
        return;
    }

    // Clear old contents.
    QByteArray array;
    bool ok;

    arrayTableWidget->setXData(new SeerArrayWidget::DataStorageArray(array));

    if (xArrayOffsetLineEdit->text() != "") {
        arrayTableWidget->setXAddressOffset(xArrayOffsetLineEdit->text().toULong(&ok));
        if (ok == false) {
            qWarning() << "Invalid string for address offset." << xArrayOffsetLineEdit->text();
        }
    }else{
        arrayTableWidget->setXAddressOffset(0);
    }

    if (xArrayStrideLineEdit->text() != "") {
        arrayTableWidget->setXAddressStride(xArrayStrideLineEdit->text().toULong(&ok));
        if (ok == false) {
            qWarning() << "Invalid string for address stride." << xArrayStrideLineEdit->text();
        }
    }else{
        arrayTableWidget->setXAddressStride(1);
    }

    // Send signal to get variable address.
    emit evaluateVariableExpression(_xVariableId, xVariableNameLineEdit->text());
}

QString SeerArrayVisualizerWidget::xVariableName () const {
    return xVariableNameLineEdit->text();
}

void SeerArrayVisualizerWidget::setXVariableAddress (const QString& address) {

    if (address.startsWith("0x")) {

        bool ok = false;

        address.toULong(&ok, 16);

        if (ok == true) {
            xVariableAddressLineEdit->setText(address);
        }else{
            xVariableAddressLineEdit->setText("not an address");
        }

    }else{
        xVariableAddressLineEdit->setText("not an address");
    }

    arrayTableWidget->setXAddressOffset(0);
}

QString SeerArrayVisualizerWidget::xVariableAddress () const {
    return xVariableAddressLineEdit->text();
}

void SeerArrayVisualizerWidget::setYVariableName (const QString& name) {

    setWindowTitle("Seer Array Visualizer - '" + name + "'");

    yVariableNameLineEdit->setText(name);
    setYVariableAddress("");

    if (yVariableNameLineEdit->text() == "") {
        return;
    }

    // Clear old contents.
    QByteArray array;
    bool ok;

    arrayTableWidget->setYData(new SeerArrayWidget::DataStorageArray(array));

    if (yArrayOffsetLineEdit->text() != "") {
        arrayTableWidget->setXAddressOffset(yArrayOffsetLineEdit->text().toULong(&ok));
        if (ok == false) {
            qWarning() << "Invalid string for address offset." << yArrayOffsetLineEdit->text();
        }
    }else{
        arrayTableWidget->setXAddressOffset(0);
    }

    if (yArrayStrideLineEdit->text() != "") {
        arrayTableWidget->setXAddressStride(yArrayStrideLineEdit->text().toULong(&ok));
        if (ok == false) {
            qWarning() << "Invalid string for address stride." << yArrayStrideLineEdit->text();
        }
    }else{
        arrayTableWidget->setXAddressStride(1);
    }

    // Send signal to get variable address.
    emit evaluateVariableExpression(_yVariableId, yVariableNameLineEdit->text());
}

QString SeerArrayVisualizerWidget::yVariableName () const {
    return yVariableNameLineEdit->text();
}

void SeerArrayVisualizerWidget::setYVariableAddress (const QString& address) {

    if (address.startsWith("0x")) {

        bool ok = false;

        address.toULong(&ok, 16);

        if (ok == true) {
            yVariableAddressLineEdit->setText(address);
        }else{
            yVariableAddressLineEdit->setText("not an address");
        }

    }else{
        yVariableAddressLineEdit->setText("not an address");
    }

    arrayTableWidget->setXAddressOffset(0);
}

QString SeerArrayVisualizerWidget::yVariableAddress () const {
    return yVariableAddressLineEdit->text();
}

void SeerArrayVisualizerWidget::handleText (const QString& text) {

    //qDebug() << text;

    if (text.contains(QRegExp("^([0-9]+)\\^done,value="))) {

        // 10^done,value="1"
        // 11^done,value="0x7fffffffd538"

        QString id_text = text.section('^', 0,0);

        if (id_text.toInt() == _xVariableId) {

            QStringList words = Seer::filterEscapes(Seer::parseFirst(text, "value=", '"', '"', false)).split(' ', Qt::SkipEmptyParts);

            setXVariableAddress(words.first());
        }

        if (id_text.toInt() == _yVariableId) {

            QStringList words = Seer::filterEscapes(Seer::parseFirst(text, "value=", '"', '"', false)).split(' ', Qt::SkipEmptyParts);

            setYVariableAddress(words.first());
        }

    }else if (text.contains(QRegExp("^([0-9]+)\\^done,memory="))) {

        // 3^done,memory=[{begin="0x0000000000613e70",offset="0x0000000000000000",end="0x0000000000613e71",contents="00"}]
        // 4^done,memory=[{begin="0x0000000000613e70",offset="0x0000000000000000",end="0x0000000000613ed4",contents="000000000000000000000000"}]

        QString id_text = text.section('^', 0,0);

        if (id_text.toInt() == _xMemoryId) {

            //qDebug() << text;

            QString memory_text = Seer::parseFirst(text, "memory=", '[', ']', false);

            QStringList range_list = Seer::parse(memory_text, "", '{', '}', false);

            // Loop through the memory ranges.
            for ( const auto& range_text : range_list  ) {

                QString contents_text = Seer::parseFirst(range_text, "contents=", '"', '"', false);

                //qDebug() << contents_text;

                // Convert hex string to byte array.
                QByteArray array;

                for (int i = 0; i<contents_text.size(); i += 2) {
                    QString num = contents_text.mid(i, 2);
                    bool ok = false;
                    array.push_back(num.toInt(&ok, 16));
                    Q_ASSERT(ok);
                }

                // Give the byte array to the hex widget.
                bool ok;
                arrayTableWidget->setXData(new SeerArrayWidget::DataStorageArray(array));

                if (xArrayOffsetLineEdit->text() != "") {
                    arrayTableWidget->setXAddressOffset(xArrayOffsetLineEdit->text().toULong(&ok));
                    if (ok == false) {
                        qWarning() << "Invalid string for address offset." << xArrayOffsetLineEdit->text();
                    }
                }else{
                    arrayTableWidget->setXAddressOffset(0);
                }

                if (xArrayStrideLineEdit->text() != "") {
                    arrayTableWidget->setXAddressStride(xArrayStrideLineEdit->text().toULong(&ok));
                    if (ok == false) {
                        qWarning() << "Invalid string for address stride." << xArrayStrideLineEdit->text();
                    }
                }else{
                    arrayTableWidget->setXAddressStride(1);
                }

                break; // Take just the first range for now.
            }
        }

        if (id_text.toInt() == _yMemoryId) {

            //qDebug() << text;

            QString memory_text = Seer::parseFirst(text, "memory=", '[', ']', false);

            QStringList range_list = Seer::parse(memory_text, "", '{', '}', false);

            // Loop through the memory ranges.
            for ( const auto& range_text : range_list  ) {

                QString contents_text = Seer::parseFirst(range_text, "contents=", '"', '"', false);

                //qDebug() << contents_text;

                // Convert hex string to byte array.
                QByteArray array;

                for (int i = 0; i<contents_text.size(); i += 2) {
                    QString num = contents_text.mid(i, 2);
                    bool ok = false;
                    array.push_back(num.toInt(&ok, 16));
                    Q_ASSERT(ok);
                }

                // Give the byte array to the hex widget.
                bool ok;
                arrayTableWidget->setYData(new SeerArrayWidget::DataStorageArray(array));

                if (yArrayOffsetLineEdit->text() != "") {
                    arrayTableWidget->setYAddressOffset(yArrayOffsetLineEdit->text().toULong(&ok));
                    if (ok == false) {
                        qWarning() << "Invalid string for address offset." << yArrayOffsetLineEdit->text();
                    }
                }else{
                    arrayTableWidget->setYAddressOffset(0);
                }

                if (yArrayStrideLineEdit->text() != "") {
                    arrayTableWidget->setYAddressStride(yArrayStrideLineEdit->text().toULong(&ok));
                    if (ok == false) {
                        qWarning() << "Invalid string for address stride." << yArrayStrideLineEdit->text();
                    }
                }else{
                    arrayTableWidget->setYAddressStride(1);
                }

                break; // Take just the first range for now.
            }
        }

    }else if (text.contains(QRegExp("^([0-9]+)\\^error,msg="))) {

        // 12^error,msg="No symbol \"return\" in current context."
        // 13^error,msg="No symbol \"cout\" in current context."
        // 3^error,msg="Unable to read memory."

        QString id_text = text.section('^', 0,0);

        if (id_text.toInt() == _xVariableId) {
            xVariableAddressLineEdit->setText( Seer::filterEscapes(Seer::parseFirst(text, "msg=", '"', '"', false)) );
        }

        if (id_text.toInt() == _xMemoryId) {
            // Display the error message.
            QString msg_text = Seer::parseFirst(text, "msg=", false);

            if (msg_text != "") {
                QMessageBox::warning(this, "Error.", Seer::filterEscapes(msg_text));
            }
        }

        if (id_text.toInt() == _yVariableId) {
            yVariableAddressLineEdit->setText( Seer::filterEscapes(Seer::parseFirst(text, "msg=", '"', '"', false)) );
        }

        if (id_text.toInt() == _yMemoryId) {
            // Display the error message.
            QString msg_text = Seer::parseFirst(text, "msg=", false);

            if (msg_text != "") {
                QMessageBox::warning(this, "Error.", Seer::filterEscapes(msg_text));
            }
        }

    }else{
        // Ignore anything else.
    }
}

void SeerArrayVisualizerWidget::handlexRefreshButton () {

    if (xVariableNameLineEdit->text() == "") {
        return;
    }

    if (xVariableAddressLineEdit->text() == "") {
        return;
    }

    if (xVariableAddressLineEdit->text() == "not an address") {
        return;
    }

    int bytes = xArrayLengthLineEdit->text().toInt() * Seer::typeBytes(xArrayDisplayFormatComboBox->currentText());

    //qDebug() << _xMemoryId << xVariableAddressLineEdit->text() << xArrayLengthLineEdit->text() << xArrayDisplayFormatComboBox->currentText() << bytes;

    emit evaluateMemoryExpression(_xMemoryId, xVariableAddressLineEdit->text(), bytes);
}

void SeerArrayVisualizerWidget::handleyRefreshButton () {

    if (yVariableNameLineEdit->text() == "") {
        return;
    }

    if (yVariableAddressLineEdit->text() == "") {
        return;
    }

    if (yVariableAddressLineEdit->text() == "not an address") {
        return;
    }

    int bytes = yArrayLengthLineEdit->text().toInt() * Seer::typeBytes(yArrayDisplayFormatComboBox->currentText());

    //qDebug() << _yMemoryId << yVariableAddressLineEdit->text() << yArrayLengthLineEdit->text() << yArrayDisplayFormatComboBox->currentText() << bytes;

    emit evaluateMemoryExpression(_yMemoryId, yVariableAddressLineEdit->text(), bytes);
}

void SeerArrayVisualizerWidget::handlexVariableNameLineEdit () {

    setXVariableName (xVariableNameLineEdit->text());
}

void SeerArrayVisualizerWidget::handleyVariableNameLineEdit () {

    setYVariableName (yVariableNameLineEdit->text());
}

void SeerArrayVisualizerWidget::handlexArrayDisplayFormatComboBox (int index) {

    //qDebug() << index;

    if (index == 0) {
        arrayTableWidget->setXArrayMode(SeerArrayWidget::Int16ArrayMode);

    }else if (index == 1) {
        arrayTableWidget->setXArrayMode(SeerArrayWidget::Int32ArrayMode);

    }else if (index == 2) {
        arrayTableWidget->setXArrayMode(SeerArrayWidget::Int64ArrayMode);

    }else if (index == 3) {
        arrayTableWidget->setXArrayMode(SeerArrayWidget::UInt16ArrayMode);

    }else if (index == 4) {
        arrayTableWidget->setXArrayMode(SeerArrayWidget::UInt32ArrayMode);

    }else if (index == 5) {
        arrayTableWidget->setXArrayMode(SeerArrayWidget::UInt64ArrayMode);

    }else if (index == 6) {
        arrayTableWidget->setXArrayMode(SeerArrayWidget::Float32ArrayMode);

    }else if (index == 7) {
        arrayTableWidget->setXArrayMode(SeerArrayWidget::Float64ArrayMode);

    }else{
        // Do nothing.
    }
}

void SeerArrayVisualizerWidget::handleyArrayDisplayFormatComboBox (int index) {

    //qDebug() << index;

    if (index == 0) {
        arrayTableWidget->setYArrayMode(SeerArrayWidget::Int16ArrayMode);

    }else if (index == 1) {
        arrayTableWidget->setYArrayMode(SeerArrayWidget::Int32ArrayMode);

    }else if (index == 2) {
        arrayTableWidget->setYArrayMode(SeerArrayWidget::Int64ArrayMode);

    }else if (index == 3) {
        arrayTableWidget->setYArrayMode(SeerArrayWidget::UInt16ArrayMode);

    }else if (index == 4) {
        arrayTableWidget->setYArrayMode(SeerArrayWidget::UInt32ArrayMode);

    }else if (index == 5) {
        arrayTableWidget->setYArrayMode(SeerArrayWidget::UInt64ArrayMode);

    }else if (index == 6) {
        arrayTableWidget->setYArrayMode(SeerArrayWidget::Float32ArrayMode);

    }else if (index == 7) {
        arrayTableWidget->setYArrayMode(SeerArrayWidget::Float64ArrayMode);

    }else{
        // Do nothing.
    }
}

void SeerArrayVisualizerWidget::handleDataChanged () {

    if (_series) {
        arrayChartView->chart()->removeSeries(_series);
        delete _series;
        _series = 0;
    }

    if (scatterRadioButton->isChecked()) {

        QScatterSeries* scatter = new QScatterSeries;
        scatter->setMarkerSize(7);
        _series = scatter;

    }else if (lineRadioButton->isChecked()) {

        QLineSeries* line = new QLineSeries;
        _series = line;

    }else if (splineRadioButton->isChecked()) {

        QSplineSeries* line  = new QSplineSeries;
        _series = line;
    }

    if (_series == 0) {
        return;
    }

    _series->setName(xVariableName());
    _series->setPointsVisible(false);
    _series->setPointLabelsVisible(false);

    if (arrayTableWidget->xSize() > 0 && arrayTableWidget->ySize() == 0) {

        const QVector<double>& values = arrayTableWidget->xArrayValues();

        for (int i = 0; i < values.size(); ++i) {
            _series->append(i, values[i]);
        }

    } else if (arrayTableWidget->xSize() > 0 && arrayTableWidget->ySize() > 0) {

        const QVector<double>& xvalues = arrayTableWidget->xArrayValues();
        const QVector<double>& yvalues = arrayTableWidget->yArrayValues();

        for (int i = 0; i < std::min(xvalues.size(),yvalues.size()); ++i) {
            _series->append(xvalues[i], yvalues[i]);
        }
    }

    QObject::connect(_series, &QLineSeries::hovered,     this, &SeerArrayVisualizerWidget::handleSeriesHovered);

    arrayChartView->chart()->addSeries(_series);
    arrayChartView->chart()->createDefaultAxes();
}

void SeerArrayVisualizerWidget::writeSettings() {

    QSettings settings;

    settings.beginGroup("arrayvisualizerwindow"); {
        settings.setValue("size", size());
        settings.setValue("splitter", splitter->saveState());
    } settings.endGroup();

    //qDebug() << size();
}

void SeerArrayVisualizerWidget::readSettings() {

    QSettings settings;

    settings.beginGroup("arrayvisualizerwindow"); {
        resize(settings.value("size", QSize(800, 400)).toSize());
        splitter->restoreState(settings.value("splitter").toByteArray());
    } settings.endGroup();

    //qDebug() << size();
}

void SeerArrayVisualizerWidget::resizeEvent (QResizeEvent* event) {

    writeSettings();

    QWidget::resizeEvent(event);
}

void SeerArrayVisualizerWidget::handleSplitterMoved (int pos, int index) {

    Q_UNUSED(pos);
    Q_UNUSED(index);

    writeSettings();
}

void SeerArrayVisualizerWidget::handleSeriesHovered (const QPointF& point, bool state) {

    //qDebug() << "QPointF=" << point << "State=" << state << "MapToPosition=" << arrayChartView->chart()->mapToPosition(point) << "MapFromScene=" << arrayChartView->mapFromScene(arrayChartView->chart()->mapToPosition(point));

    if (state) {
        QToolTip::showText(arrayChartView->mapToGlobal(arrayChartView->mapFromScene(arrayChartView->chart()->mapToPosition(point))), QString("%1 / %2").arg(point.x()).arg(point.y()), this, QRect(), 10000);
    }else{
        QToolTip::hideText();
    }
}

void SeerArrayVisualizerWidget::handleTitleLineEdit () {

    arrayChartView->chart()->setTitle(titleLineEdit->text());

    titleLineEdit->setText("");
}

void SeerArrayVisualizerWidget::handlePointsCheckBox () {

    if (_series) {
        _series->setPointsVisible(pointsCheckBox->isChecked());
    }
}

void SeerArrayVisualizerWidget::handleLabelsCheckBox () {

    if (_series) {
        _series->setPointLabelsVisible(labelsCheckBox->isChecked());
    }
}

void SeerArrayVisualizerWidget::handleLineTypeButtonGroup () {

    handleDataChanged();
}

