// Plot.c
//
// Generic plot function called by the control panel script system.
// When the plot is created the following should be defined:
//  - Title
//  - Yaxis title
//  - Xaxis title
//  - Number of plot, 1 or 2 for now
// A generic PlotCommand function accepts plot command string to enable
// ploting data. All commands sent to the plot command function are
// appended to the comments.
//
// Add the following capability:
//  - Comment system, view and edit comments
//  - Add popup menu system that will have the following functions
//      - Save
//      - Load
//      - trace
//      - X axis zoom
//      - Y axis zoom
//      - Edit comments
//      - Display heatmap of scans
//  - Add strip chart option
//
#include "plot.h"
#include "ui_plot.h"

SGcoeff SG[4] = {{5,35,QList<float>() << -3 << 12 << 17 << 12 << -3},
                 {7,21,QList<float>() << -2 << 3 << 6 << 7 << 6 << 3 << -2},
                 {9,231,QList<float>() << -21 << 14 << 39 << 54 << 59 << 54 <<39 << 14 << -21},
                 {11,429,QList<float>() << -36 << 9 << 44 << 69 << 84 << 89 <<84 << 69 << 44 << 9 << -36}};
// =(-2*A1+3*A2+6*A3+7*A4+6*A5+3*A6-2*A7)/21
// =(-21*A1+14*A2+39*A3+54*A4+59*A5+54*A6+39*A7+14*A8-21*A9)/231
// =(-36*A1+9*A2+44*A3+69*A4+84*A5+89*A6+84*A7+69*A8+44*A9+9*A10-39*A11)/429
Plot::Plot(QWidget *parent, QString Title, QString Yaxis, QString Xaxis, int NumPlots) :
    QDialog(parent),
    ui(new Ui::Plot)
{
    ui->setupUi(this);
    Qt::WindowFlags flags = this->windowFlags();
    //    this->setWindowFlags(flags | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint);
    //    this->setWindowFlags(flags | Qt::WindowStaysOnTopHint);
    m=1;
    b=0;
    Filter = -1;
    ui->txtComments->appendPlainText("CreatePlot," + Title.replace(",","_") + "," + Yaxis.replace(",","_") + "," + Xaxis.replace(",","_") + "," + QString::number(NumPlots));
    PlotTitle = Title;
    Plot::setWindowTitle(Title);
    this->installEventFilter(this);
    ui->txtComments->setVisible(false);
    ui->pbCloseComments->setVisible(false);
    plotGraphs.clear();
    // Setup the statusbar
    statusBar = new QStatusBar(this);
    statusBar->setGeometry(0,this->height()-18,this->width(),18);
    statusBar->raise();
    // Hide the heat maps
    ui->HeatMap1->setVisible(false);
    ui->HeatMap2->setVisible(false);
    colorScale1 = new QCPColorScale(ui->HeatMap1);
    colorMap1   = new QCPColorMap(ui->HeatMap1->xAxis, ui->HeatMap1->yAxis);
    colorScale2 = new QCPColorScale(ui->HeatMap2);
    colorMap2   = new QCPColorMap(ui->HeatMap2->xAxis, ui->HeatMap2->yAxis);
    // give the axes some labels:
    ui->Graph->xAxis->setLabel(Xaxis);
    ui->Graph->yAxis->setLabel(Yaxis);
    // create the plots
    for(int i=0; i<NumPlots; i++)
    {
        ui->Graph->addGraph();
        //if(i == 0) ui->Graph->graph(0)->setPen(QPen(Qt::blue));
        if(i == 1) ui->Graph->graph(1)->setPen(QPen(Qt::red));
    }
    ui->Graph->replot();
    connect(ui->Graph, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(mousePressed(QMouseEvent*)));
    connect(ui->Graph, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(mouseMove(QMouseEvent*)));
    connect(ui->HeatMap1, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(mousePressedHM(QMouseEvent*)));
    connect(ui->HeatMap2, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(mousePressedHM(QMouseEvent*)));
    connect(ui->pbCloseComments, SIGNAL(pressed()), this, SLOT(slotCloseComments()));
    // Plot popup menu options
    SaveOption = new QAction("Save", this);
    connect(SaveOption, SIGNAL(triggered()), this, SLOT(slotSaveMenu()));
    LoadOption = new QAction("Load", this);
    connect(LoadOption, SIGNAL(triggered()), this, SLOT(slotLoadMenu()));
    CommentOption  = new QAction("Comments", this);
    connect(CommentOption, SIGNAL(triggered()), this, SLOT(slotCommentMenu()));
    XaxisZoomOption = new QAction("Xaxis zoom", this);
    XaxisZoomOption->setCheckable(true);
    connect(XaxisZoomOption, SIGNAL(triggered()), this, SLOT(slotXaxisZoomOption()));
    YaxisZoomOption = new QAction("Yaxis zoom", this);
    YaxisZoomOption->setCheckable(true);
    connect(YaxisZoomOption, SIGNAL(triggered()), this, SLOT(slotYaxisZoomOption()));
    FilterOption = new QAction("Filter", this);
    connect(FilterOption, SIGNAL(triggered()), this, SLOT(slotFilterOption()));
    TrackOption = new QAction("Track", this);
    TrackOption->setCheckable(true);
    connect(TrackOption, SIGNAL(triggered()), this, SLOT(slotTrackOption()));
    ClipboardOption = new QAction("Clipboard", this);
    connect(ClipboardOption, SIGNAL(triggered()), this, SLOT(slotClipBoard()));
    HeatOption = new QAction("Heatmap", this);
    HeatOption->setCheckable(true);
    connect(HeatOption, SIGNAL(triggered()), this, SLOT(slotHeatMap()));
}

void Plot::FreeAllData(void)
{
    for(int i=0;i<plotGraphs.count();i++)
    {
        for(int j=0;j<plotGraphs[i]->Vec.count();j++)
        {
            for(int k=0;k<plotGraphs[i]->Vec[j]->Y.count();k++)
            {
                delete plotGraphs[i]->Vec[j]->Y[k];
            }
        }
        plotGraphs[i]->Vec.clear();
    }
    plotGraphs.clear();
}

Plot::~Plot()
{
    FreeAllData();
    delete ui;
}

void Plot::resizeEvent(QResizeEvent *event)
{
    ui->Graph->setGeometry(0,0,this->width(),this->height()-18);
    ui->HeatMap1->setGeometry(0,0,this->width()/2,this->height()-18);
    ui->HeatMap2->setGeometry(this->width()/2,0,this->width()/2,this->height()-18);
    statusBar->setGeometry(0,this->height()-18,this->width(),18);
    ui->txtComments->setGeometry(0,0,this->width()-ui->pbCloseComments->width(),this->height()-18);
    ui->pbCloseComments->setGeometry(this->width()-ui->pbCloseComments->width(),this->height()-18-ui->pbCloseComments->height(),ui->pbCloseComments->width(),ui->pbCloseComments->height());
    ui->Graph->replot();
    QWidget::resizeEvent(event);
}

bool Plot::eventFilter(QObject *obj, QEvent *event)
{

    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *key = static_cast<QKeyEvent *>(event);
        if(key->key() == 16777236)  // Right arrow
        {
            CurrentIndex++;
            if(CurrentIndex >= plotGraphs.count()) CurrentIndex = plotGraphs.count()-1;
            if(CurrentIndex >= 0)
            {
                PaintGraphs(plotGraphs[CurrentIndex]);
                Plot::setWindowTitle(PlotTitle + ", " + "number: " + QString::number(CurrentIndex+1));
                ui->Graph->replot();
            }
            return true;
        }
        if(key->key() == 16777234) // Left arrow
        {
            CurrentIndex--;
            if(CurrentIndex < 0) CurrentIndex = 0;
            if((CurrentIndex >= 0) && (plotGraphs.count() > 0))
            {
                PaintGraphs(plotGraphs[CurrentIndex]);
                Plot::setWindowTitle(PlotTitle + ", " + "number: " + QString::number(CurrentIndex+1));
                ui->Graph->replot();
            }
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

void Plot::mousePressedHM(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
       popupMenu = new QMenu(tr("Plot options"), this);
       popupMenu->addAction(HeatOption);
       popupMenu->exec(event->globalPos());
    }
}

void Plot::mouseMove(QMouseEvent*event)
{
    if(TrackOption->isChecked())
    {
        double x = ui->Graph->xAxis->pixelToCoord(event->pos().x());
        double y = ui->Graph->yAxis->pixelToCoord(event->pos().y());
        statusBar->showMessage("X = " + QString::number(x) + ", Y = " + QString::number(y));
    }
}

void Plot::mousePressed(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
       popupMenu = new QMenu(tr("Plot options"), this);
       popupMenu->addAction(SaveOption);
       popupMenu->addAction(LoadOption);
       popupMenu->addAction(XaxisZoomOption);
       popupMenu->addAction(YaxisZoomOption);
       popupMenu->addAction(FilterOption);
       popupMenu->addAction(TrackOption);
       popupMenu->addAction(ClipboardOption);
       popupMenu->addAction(CommentOption);
       popupMenu->addAction(HeatOption);
       popupMenu->exec(event->globalPos());
    }
}

// Save all the comments to a text file. The comments include
// all the plot commands.
void Plot::Save(QString filename)
{
    if(filename.isEmpty()) return;
    QFile file(filename);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // We're going to streaming text to the file
        QTextStream stream(&file);
        stream << ui->txtComments->toPlainText();
        file.close();
    }
}

void Plot::slotSaveMenu(void)
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save plot data","","Plot (*.plot);;All files (*.*)");
    Save(fileName);
}

void Plot::slotLoadMenu(void)
{
    QString fileName = QFileDialog::getOpenFileName(this, "Load plot data","","Plot (*.plot);;All files (*.*)");
    Load(fileName);
}

void Plot::Load(QString filename)
{
    if(filename.isEmpty()) return;
    QFile file(filename);
    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        // We're going to streaming the file
        // to a QString
        QTextStream stream(&file);
        Comments = stream.readAll();
        ui->txtComments->clear();
        ui->txtComments->appendPlainText(Comments);
        // Clear and current data
        FreeAllData();
        file.close();
        // process the commands
        QStringList Lines = Comments.split("\n");
        for(int i=0;i<Lines.count();i++) PlotCommand(Lines[i],true);
    }
}

void Plot::slotCommentMenu(void)
{
    if(!ui->txtComments->isVisible())
    {
        ui->Graph->setVisible(false);
        ui->txtComments->setVisible(true);
        ui->pbCloseComments->setVisible(true);
    }
}

void Plot::slotCloseComments(void)
{
    ui->Graph->setVisible(true);
    ui->txtComments->setVisible(false);
    ui->pbCloseComments->setVisible(false);
}

//Savitzky-Golay filter
void Plot::SavitzkyGolayFilter(int order, QList<float> Y, QList<float> *Yf)
{
   float   val;
   int     k;
   SGcoeff sg;

   (*Yf).clear();
   if(order<0)
   {
       for(int i=0;i<Y.count();i++) (*Yf).append(Y[i]);
       return;
   }
   sg = SG[order];
   for(int i=0;i<Y.count();i++)
   {
       val = 0;
       for(int j=-(sg.np-1)/2;j<=(sg.np-1)/2;j++)
       {
           k = abs(i+j);
           if(k >= Y.count()) k = (2*Y.count()-1) -k;
           val += Y[k] * sg.an[j+(sg.np-1)/2];
       }
       (*Yf).append(val/sg.h);
   }
}

void Plot::PaintGraphs(PlotGraph *pg)
{
    for(int i=0;i<ui->Graph->graphCount();i++) ui->Graph->graph(i)->data()->clear();
    // Filter the data
    QList<float> I,A,B;
    if(ui->Graph->graphCount()>=1)
    {
        I.clear();
        for(int i=0;i<pg->Vec.count();i++) I.append(*pg->Vec[i]->Y[0]/pg->NumScans);
        SavitzkyGolayFilter(Filter,I,&A);
    }
    if(ui->Graph->graphCount()>1)
    {
        I.clear();
        for(int i=0;i<pg->Vec.count();i++) I.append(*pg->Vec[i]->Y[1]/pg->NumScans);
        SavitzkyGolayFilter(Filter,I,&B);
    }
    // Add the data points from last plotGraphs
    for(int i=0;i<pg->Vec.count();i++)
    {
        if(ui->Graph->graphCount()>=1) ui->Graph->graph(0)->addData((pg->Vec[i]->X - pg->Vec[0]->X)*m+b, A[i]);
        if(ui->Graph->graphCount()>1) ui->Graph->graph(1)->addData((pg->Vec[i]->X - pg->Vec[0]->X)*m+b, B[i]);
    }
//    for(int i=0;i<pg->Vec.count();i++)
//    {
//        for(int j=0;j<ui->Graph->graphCount();j++)
//            ui->Graph->graph(j)->addData((pg->Vec[i]->X - pg->Vec[0]->X)*m+b, *pg->Vec[i]->Y[j]/pg->NumScans);
//    }
}

// This function accepts plot command strings. The following commands are
// supported:
// Clear
// Refresh
// Xrange, min, max
// Yrange, min, max
// NewGraph,numgraphs,points
// AddPoint,point num, X axis value, Graph1, Graph 2, ....
// Plot
// Save,filename
// Load,filename
void Plot::PlotCommand(QString cmd, bool PlotOnly)
{
    QStringList reslist = cmd.split(",");

    if(!PlotOnly)
    {
        ui->txtComments->appendPlainText(cmd);
        if(reslist[0].toUpper() == "SAVE") Save(reslist[1]);
        else if(reslist[0].toUpper() == "LOAD") Load(reslist[1]);
    }
    if((cmd.toUpper().startsWith("CREATEPLOT")) && (reslist.count()==5))
    {
        PlotTitle = reslist[1].replace("_",",");
        ui->Graph->xAxis->setLabel(reslist[3].replace("_",","));
        ui->Graph->yAxis->setLabel(reslist[2].replace("_",","));
        ui->Graph->clearGraphs();
        for(int i=0; i<reslist[4].toInt(); i++)
        {
            ui->Graph->addGraph();
            //if(i == 0) ui->Graph->graph(0)->setPen(QPen(Qt::blue));
            if(i == 1) ui->Graph->graph(1)->setPen(QPen(Qt::red));
        }
    }
    else if(reslist[0].toUpper() == "NORMALCURSOR")
    {
        QApplication::restoreOverrideCursor();
    }
    else if(reslist[0].toUpper() == "REFRESH")
    {
        if(plotGraphs.count() > 0) Plot::setWindowTitle(PlotTitle + ", " + "number: " + QString::number(CurrentIndex+1));
        else Plot::setWindowTitle(PlotTitle);
        ui->Graph->replot();
    }
    else if((cmd.toUpper().startsWith("SCAN")) && (reslist.count()==4))
    {
        Scan = reslist[1] + "," + reslist[2] + "," + reslist[3];
    }
    else if((cmd.toUpper().startsWith("XRANGE")) && (reslist.count()==2))
    {
        if(reslist[1].toUpper() == "TIME")
        {
            QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
            timeTicker->setTimeFormat("%h:%m:%s");
            ui->Graph->xAxis->setTicker(timeTicker);
            ui->Graph->axisRect()->setupFullAxesBox();

            // make left and bottom axes transfer their ranges to right and top axes:
            connect(ui->Graph->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->Graph->xAxis2, SLOT(setRange(QCPRange)));
        }
    }
    else if((cmd.toUpper().startsWith("XRANGE")) && (reslist.count()==3))
    {
        ui->Graph->xAxis->setRange(reslist[1].toDouble(), reslist[2].toDouble());
        ui->Graph->replot();
        m=1;
        b=0;
    }
    else if((cmd.toUpper().startsWith("PLOT1")) && (reslist.count()==2)) Label1 = reslist[1];
    else if((cmd.toUpper().startsWith("PLOT2")) && (reslist.count()==2)) Label2 = reslist[1];
    else if((cmd.toUpper().startsWith("XRANGE")) && (reslist.count()==5))
    {
        ui->Graph->xAxis->setRange(reslist[3].toDouble(), reslist[4].toDouble());
        ui->Graph->replot();
        m = (reslist[3].toFloat() - reslist[4].toFloat())/(reslist[1].toFloat()-reslist[2].toFloat());
        b = reslist[3].toFloat()  - reslist[1].toFloat()*m;
    }
    else if(cmd.toUpper().startsWith("YRANGE"))
    {
        if(reslist.count()==3)
        {
           ui->Graph->yAxis->setRange(reslist[1].toDouble(), reslist[2].toDouble());
           ui->Graph->replot();
        }
    }
    else if(cmd.toUpper() == "CLEAR")
    {
        for(int i=0;i<ui->Graph->graphCount();i++) ui->Graph->graph(i)->data()->clear();
    }
    else if((reslist[0].toUpper() == "NEWGRAPH")&&(reslist.count()==3))
    {
        PlotGraph *pg = new PlotGraph;
        plotGraphs.append(pg);
        plotGraphs.last()->NumScans = 0;
        for(int i=0;i<reslist[2].toInt();i++)
        {
            DataPoint *dp = new DataPoint;
            plotGraphs.last()->Vec.append(dp);
            plotGraphs.last()->Vec.last()->point = 0;
            plotGraphs.last()->Vec.last()->X = 0.0;
            for(int j=0;j<reslist[1].toInt();j++)
            {
                float *f = new float;
                plotGraphs.last()->Vec.last()->Y.append(f);
                *plotGraphs.last()->Vec.last()->Y.last() = 0.0;
            }
        }
    }
    else if((reslist[0].toUpper() == "PLOTPOINT")&&(reslist.count()>=3))
    {
        double key = reslist[1].toDouble();
        ui->Graph->graph(0)->addData(key, reslist[2].toDouble());
        if(reslist.count()==4) ui->Graph->graph(1)->addData(key, reslist[3].toDouble());
        ui->Graph->xAxis->setRangeUpper(key);
        ui->Graph->replot();
        this->raise();
    }
    else if((reslist[0].toUpper() == "ADDPOINT")&&(reslist.count()>=4))
    {
        if((reslist[1].toInt()-1 < plotGraphs.last()->Vec.count()) && (reslist[1].toInt() > 0))
        {
           if(reslist[1].toInt()-1 == 0) plotGraphs.last()->NumScans++;
           plotGraphs.last()->Vec[reslist[1].toInt()-1]->X = reslist[2].toFloat();
           *plotGraphs.last()->Vec[reslist[1].toInt()-1]->Y[0] += reslist[3].toFloat();
           if(reslist.count()>4) *plotGraphs.last()->Vec[reslist[1].toInt()-1]->Y[1] += reslist[4].toFloat();
        }
    }
    else if(cmd.toUpper() == "PLOT")
    {
        CurrentIndex = plotGraphs.count() - 1;
        PaintGraphs(plotGraphs.last());
    }
    else if((reslist[0].toUpper() == "TITLE")&&(reslist.count()==2))
    {
        PlotTitle = reslist[1];
        Plot::setWindowTitle(PlotTitle);
    }
}

void Plot::slotXaxisZoomOption(void)
{
    if(XaxisZoomOption->isChecked()) XaxisZoomOption->setChecked(true);
    else XaxisZoomOption->setChecked(false);
    ZoomSelect();
}

void Plot::slotYaxisZoomOption(void)
{
    if(YaxisZoomOption->isChecked()) YaxisZoomOption->setChecked(true);
    else YaxisZoomOption->setChecked(false);
    ZoomSelect();
}

void Plot::slotFilterOption(void)
{
    QMessageBox msgBox;
    bool ok;

    QString res = QInputDialog::getText(this, "Savitzky-Golay filter", "Enter Savitzky-Golay filter polynomial length, 5,7,9, or 11. \nEnter 0 for no filter.", QLineEdit::Normal,QString::null, &ok);
    if(ok && !res.isEmpty())
    {
        switch(res.toInt())
        {
           case 5:
            Filter = 0;
            break;
           case 7:
            Filter = 1;
            break;
           case 9:
            Filter = 2;
            break;
           case 11:
            Filter = 3;
            break;
           case 0:
            Filter = -1;
            break;
           default:
            msgBox.setText("Invalid entry, filter will be turned off.");
            msgBox.setInformativeText("");
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
            Filter = -1;
            break;
        }
        PaintGraphs(plotGraphs[CurrentIndex]);
        ui->Graph->replot();
    }
}

void Plot::ZoomSelect(void)
{
    if(XaxisZoomOption->isChecked() && YaxisZoomOption->isChecked())
    {
        ui->Graph->xAxis->axisRect()->setRangeZoom(Qt::Vertical | Qt::Horizontal);
        ui->Graph->xAxis->axisRect()->setRangeDrag(Qt::Vertical | Qt::Horizontal);
        ui->Graph->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    }
    else if(XaxisZoomOption->isChecked())
    {
        ui->Graph->xAxis->axisRect()->setRangeZoom(Qt::Horizontal);
        ui->Graph->xAxis->axisRect()->setRangeDrag(Qt::Horizontal);
        ui->Graph->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    }
    else if(YaxisZoomOption->isChecked())
    {
        ui->Graph->yAxis->axisRect()->setRangeZoom(Qt::Vertical);
        ui->Graph->yAxis->axisRect()->setRangeDrag(Qt::Vertical);
        ui->Graph->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    }
    else ui->Graph->setInteractions(0);
}

void Plot::slotHeatMap(void)
{
   if(plotGraphs.count() <= 0) return;
   if(plotGraphs[0]->Vec.count() <= 0) return;
   if(HeatOption->isChecked())
   {
       ui->HeatMap1->setVisible(true);
       // Heatmap 1 setup...
       // configure axis rect:
       colorMap1->data()->clear();
       ui->HeatMap1->setInteractions(QCP::iRangeDrag|QCP::iRangeZoom); // this will also allow rescaling the color scale by dragging/zooming
       ui->HeatMap1->axisRect()->setupFullAxesBox(true);
       ui->HeatMap1->xAxis->setLabel(ui->Graph->xAxis->label());
       if(Scan.isEmpty()) ui->HeatMap1->yAxis->setLabel("Scan");
       else ui->HeatMap1->yAxis->setLabel(Scan.split(",")[0]);
       // Arg was false, changed for Qt 5.9
       ui->HeatMap1->xAxis->axisRect()->setRangeDrag(Qt::Horizontal);
       ui->HeatMap1->xAxis->axisRect()->setRangeZoom(Qt::Horizontal);

       if(plotGraphs.isEmpty()) return;
       // set up the QCPColorMap:
       int nx = plotGraphs[0]->Vec.count();
       int ny = plotGraphs.count();
       colorMap1->data()->setSize(nx, ny); // we want the color map to have nx * ny data points
       colorMap1->data()->setRange(QCPRange(0, plotGraphs[0]->Vec.count()), QCPRange(0, plotGraphs.count())); // and span the coordinate range -4..4 in both key (x) and value (y) dimensions
       // now we assign some data, by accessing the QCPColorMapData instance of the color map:
       QList<float> I,A;
       for (int yIndex=0; yIndex<ny; ++yIndex)
       {
           I.clear();
           for (int xIndex=0; xIndex<nx; ++xIndex) I.append(*plotGraphs[yIndex]->Vec[xIndex]->Y[0]);
           SavitzkyGolayFilter(Filter,I,&A);
           for (int xIndex=0; xIndex<nx; ++xIndex)
           {
               colorMap1->data()->setCell(xIndex, yIndex, A[xIndex]);
           }
       }
//       for (int xIndex=0; xIndex<nx; ++xIndex)
//       {
//         for (int yIndex=0; yIndex<ny; ++yIndex)
//         {
//           colorMap1->data()->setCell(xIndex, yIndex, *plotGraphs[yIndex]->Vec[xIndex]->Y[0]);
//         }
//       }
       // add a color scale:
       ui->HeatMap1->plotLayout()->addElement(0, 1, colorScale1); // add it to the right of the main axis rect
       colorScale1->setType(QCPAxis::atRight); // scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
       colorMap1->setColorScale(colorScale1); // associate the color map with the color scale
       colorScale1->axis()->setLabel(Label1);
       colorScale1->setRangeDrag(true);
       colorScale1->setRangeZoom(true);

       // set the color gradient of the color map to one of the presets:
       colorMap1->setGradient(QCPColorGradient::gpPolar);
       // we could have also created a QCPColorGradient instance and added own colors to
       // the gradient, see the documentation of QCPColorGradient for what's possible.

       // rescale the data dimension (color) such that all data points lie in the span visualized by the color gradient:
       colorMap1->rescaleDataRange();

       // make sure the axis rect and color scale synchronize their bottom and top margins (so they line up):
       QCPMarginGroup *marginGroup = new QCPMarginGroup(ui->HeatMap1);
       ui->HeatMap1->axisRect()->setMarginGroup(QCP::msBottom|QCP::msTop, marginGroup);
       colorScale1->setMarginGroup(QCP::msBottom|QCP::msTop, marginGroup);

       int step = 5;
       // Label the X axis ticks
       QSharedPointer<QCPAxisTickerText> textTickerX(new QCPAxisTickerText);
       for(int i=0;i<=step;i++) textTickerX->addTick(i * nx/5, QString::number(ui->Graph->xAxis->range().lower + i * (ui->Graph->xAxis->range().upper - ui->Graph->xAxis->range().lower)/step));
       ui->HeatMap1->xAxis->setTicker(textTickerX);
       // Label the y axis ticks
       QSharedPointer<QCPAxisTickerText> textTickerY(new QCPAxisTickerText);
       if(!Scan.isEmpty())
       {
          textTickerY->addTick(0,Scan.split(",")[1]);
          textTickerY->addTick(ny/3,QString::number(Scan.split(",")[1].toFloat() + (Scan.split(",")[2].toFloat()-Scan.split(",")[1].toFloat())/3.0));
          textTickerY->addTick(ny*2.0/3.0,QString::number(Scan.split(",")[1].toFloat() + 2.0*(Scan.split(",")[2].toFloat()-Scan.split(",")[1].toFloat())/3.0));
          textTickerY->addTick(ny,Scan.split(",")[2]);
          ui->HeatMap1->yAxis->setTicker(textTickerY);
       }
       // rescale the key (x) and value (y) axes so the whole color map is visible:
       ui->HeatMap1->rescaleAxes();
       ui->HeatMap1->replot();

       if(plotGraphs[0]->Vec[0]->Y.count() < 2) return;
       // Heatmap 2 setup...
       ui->HeatMap2->setVisible(true);
       // configure axis rect:
       colorMap2->data()->clear();
       ui->HeatMap2->setInteractions(QCP::iRangeDrag|QCP::iRangeZoom); // this will also allow rescaling the color scale by dragging/zooming
       ui->HeatMap2->axisRect()->setupFullAxesBox(true);
       ui->HeatMap2->xAxis->setLabel(ui->Graph->xAxis->label());
       if(Scan.isEmpty()) ui->HeatMap2->yAxis->setLabel("Scan");
       else ui->HeatMap2->yAxis->setLabel(Scan.split(",")[0]);
       // Changed for Qt version 5.9
       ui->HeatMap2->xAxis->axisRect()->setRangeDrag(Qt::Horizontal);
       ui->HeatMap2->xAxis->axisRect()->setRangeZoom(Qt::Horizontal);

       // set up the QCPColorMap:
       nx = plotGraphs[0]->Vec.count();
       ny = plotGraphs.count();
       colorMap2->data()->setSize(nx, ny); // we want the color map to have nx * ny data points
       colorMap2->data()->setRange(QCPRange(0, plotGraphs[0]->Vec.count()), QCPRange(0, plotGraphs.count())); // and span the coordinate range -4..4 in both key (x) and value (y) dimensions
       // now we assign some data, by accessing the QCPColorMapData instance of the color map:
       for (int yIndex=0; yIndex<ny; ++yIndex)
       {
           I.clear();
           for (int xIndex=0; xIndex<nx; ++xIndex) I.append(*plotGraphs[yIndex]->Vec[xIndex]->Y[1]);
           SavitzkyGolayFilter(Filter,I,&A);
           for (int xIndex=0; xIndex<nx; ++xIndex)
           {
               colorMap2->data()->setCell(xIndex, yIndex, A[xIndex]);
           }
       }
//       for (int xIndex=0; xIndex<nx; ++xIndex)
//       {
//         for (int yIndex=0; yIndex<ny; ++yIndex)
//         {
//           colorMap2->data()->setCell(xIndex, yIndex, *plotGraphs[yIndex]->Vec[xIndex]->Y[1]);
//         }
//       }
       // add a color scale:
       ui->HeatMap2->plotLayout()->addElement(0, 1, colorScale2); // add it to the right of the main axis rect
       colorScale2->setType(QCPAxis::atRight); // scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
       colorMap2->setColorScale(colorScale2); // associate the color map with the color scale
       colorScale2->axis()->setLabel(Label2);
       colorScale2->setRangeDrag(true);
       colorScale2->setRangeZoom(true);

       // set the color gradient of the color map to one of the presets:
       colorMap2->setGradient(QCPColorGradient::gpPolar);
       // we could have also created a QCPColorGradient instance and added own colors to
       // the gradient, see the documentation of QCPColorGradient for what's possible.

       // rescale the data dimension (color) such that all data points lie in the span visualized by the color gradient:
       colorMap2->rescaleDataRange();

       // make sure the axis rect and color scale synchronize their bottom and top margins (so they line up):
       QCPMarginGroup *marginGroup2 = new QCPMarginGroup(ui->HeatMap2);
       ui->HeatMap2->axisRect()->setMarginGroup(QCP::msBottom|QCP::msTop, marginGroup2);
       colorScale2->setMarginGroup(QCP::msBottom|QCP::msTop, marginGroup2);

       step = 5;
       // Label the X axis ticks, drift time axis
       QSharedPointer<QCPAxisTickerText> textTickerX2(new QCPAxisTickerText);
       for(int i=0;i<=step;i++) textTickerX2->addTick(i * nx/5, QString::number(ui->Graph->xAxis->range().lower + i * (ui->Graph->xAxis->range().upper - ui->Graph->xAxis->range().lower)/step));
       ui->HeatMap2->xAxis->setTicker(textTickerX2);
       // Label the y axis ticks
       QSharedPointer<QCPAxisTickerText> textTickerY2(new QCPAxisTickerText);
       if(!Scan.isEmpty())
       {
          textTickerY2->addTick(0,Scan.split(",")[1]);
          textTickerY2->addTick(ny/3,QString::number(Scan.split(",")[1].toFloat() + (Scan.split(",")[2].toFloat()-Scan.split(",")[1].toFloat())/3.0));
          textTickerY2->addTick(ny*2.0/3.0,QString::number(Scan.split(",")[1].toFloat() + 2.0*(Scan.split(",")[2].toFloat()-Scan.split(",")[1].toFloat())/3.0));
          textTickerY2->addTick(ny,Scan.split(",")[2]);
          ui->HeatMap2->yAxis->setTicker(textTickerY2);
       }
       // rescale the key (x) and value (y) axes so the whole color map is visible:
       ui->HeatMap2->rescaleAxes();
       ui->HeatMap2->replot();
   }
   else
   {
       ui->HeatMap1->setVisible(false);
       ui->HeatMap2->setVisible(false);
   }
}

void Plot::slotTrackOption(void)
{
    if(TrackOption->isChecked()) TrackOption->setChecked(true);
    else TrackOption->setChecked(false);
}

void Plot::slotClipBoard(void)
{
    // Set the clilpboard image
    QClipboard * clipboard = QApplication::clipboard();
    QPalette mypalette=this ->palette();
    mypalette.setColor(QPalette::Window,Qt::white);
    ui->Graph->setPalette(mypalette);
    QPixmap pixmap= QPixmap::grabWidget(ui->Graph);
    clipboard->setPixmap(pixmap);
}

