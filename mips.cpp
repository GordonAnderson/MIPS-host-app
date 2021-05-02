//
// MIPS
//
// This application is desiged to communicate with the MIPS system using a USB commection
// to the MIPS system. The MIPS system is controlled using a Arduino Due and the USB
// connection can be made to the native port or the programming port. If using the native
// port then the comm parameters are not important, if using the programming port set
// the baud rate to 115200, 8 bits, no parity, and 1 stop bit.
//
// This application support control and monitoring of most MIPS function for DCbias and
// digital IO function. Support is provided to create and edit pulse sequences.
//
// Note!
//  Opening the serial port on the native USB connection to the DUE and sentting the
//  data terminal ready line to false will erase the DUE flash.
//          serial->setDataTerminalReady(false);
//
// Gordon Anderson
//
//  Revision history:
//  1.0, July 6, 2015
//      1.) Initial release
//  1.1, July 8, 2015
//      1.) Added MIPS firmware download function. Works on PC and MAC
//      2.) Removed the 1200 baud rate and all rates below 9600.
//  1.2, July 12, 2015
//      1.) Fixed the bugs in the firmware download feature.
//      2.) Tested on MAC and PC
//      3.) Updated a lot of screen issues from PC to MAC. Most features tested.
//  1.3, November 30, 2015
//      1.) Added socket support for networked MIPS.
//  1.4, January 10, 2016
//      1.) Added MIPS firmware save and boot bit setting functions
//  1.5, February 17, 2016
//      1.) Fixed bug in DC bias page that caused control is focus to update with old value
//          when returning to page.
//      2.) Fixed but that did not allow all 16 values to be edited on DC bias page
//      3.) Fixed range of DC bias page to reflect offset value
//  1.6, June 27, 2016
//      1.) Started refactoring the code. Created the comms class and moved all communications to this class.
//      2.) Add the Twave class and tab.
//      3.) Still to do, create classes for each module and finish refactoring.
//  1.7, Sept 16, 2016
//      1.) Updated the help files
//      2.) Added new clock modes to PSG
//      3.) Added tool tip popup text
//  1.8, Oct 16, 2016
//      1.) Added fwr/rev to twave options
//      2.) Added Sweep functions to Twave tab
//      3.) Updated the MIPS commands help function
//      4.) Updated the Twave help data
//      5.) Add frequency control to DIO page
// 1.9, March 7, 2017
//      1.) Added the FAIMS tab and the CV parking capability
//      2.) Updated help text
//      3.) Updated MIPS command summary
// 1.10, April 9, 2017
//      1.) Added the Single funnel control panel
//      2.) Added the Soft landing control panel
//      3.) Allow DCbias mould to auto update
//      4.) Added filament tab
//      5.) Changed connect to only show supported tab after connecting
// 1.11, April 29, 2017
//      1.) Updated ARB diaplog to support 4 ARB modules. (not tested)
//      2.) Need to make ARB context sensitive help with the multi-pass table functions
// 1.12, May 9, 2017
//      1.) Updated the softlanding form. Add the mass units and a number of minor improvments
//          requested by the customer.
// 1.13, June 24, 2017
//      1.) Added support for multiple MIPS systems. This new farmwork will allow control
//          panels that communicate to multiple MIPS systems.
//      2.) Updated the serial port scanning
//      3.) Changed the disconnection procedure
// 1.14, July 24, 2017
//      1.) Added a number of host interface commands to allow sending and receiving files
//          to SD card as well as sending and receiving EEPROM module configureation data.
// 1.15, August 22, 2017
//      1.) Added the ramp up and down pulse generation to the sequence generator
//      2.) Fixed a few minor bugs
//      3.) Fixed bug in loop timepoint calcualtation for sequence generator
// 1.16, November 21, 2017
//      1.) Updated the grid fragmentor and added the plotting capability
//      2.) Added the softlanding control panel
// 1.17, November 25, 2017
//      1.) Added the pulse sequence generator to the gid control panel
// 1.18, December 12, 2017
//      1.) Added ADC tab and data collection capability
//      2.) Updated grid pse
// 1.19, December 27, 2017
//      1.) Updated the pulse sequence generator in the Grid control panel as follows:
//             - Enabled Ts1 set to zero to stop the pulse
//             - When pulses are stopped the outputs are now driven high
// 1.20, January 1, 2018
//      1.) Inverted the shutter signals in the Grid pulse sequence generator
//      2.) Added the Ts1=Ts2 and Ts2=Ts1 check boxes
// 1.21, February 18, 2018
//      1.) Added scripting capability to Purdue softlanding control panel
// 1.22, March 11, 2018
//      1.) Added the support for programming the Flash on the ARB
//      2.) Added up and down arrow control of values for the DB bias
//          and Twave values. Also added limits for the Twave values
//          specifically the pulse voltage
// 1.23, March 12, 2018
//      1.) Added shift+alt arrow to decrease increment by 10x.
//      2.) Fixed limit bug in Twave freq in arrow keys
// 1.24, May 12, 2018
//      1.) Added the group capability for DC bias voltages
// 1.25, August 2, 2018
//      1.) Added the user definable control panels
// 1.26, Sept 9,2018
//      1.) Added documentation and updated help
//      2.) Corrected PSG label errors and make the PSE box modal
//      3.) Added Java Scripting
// 1.27, Oct 5,2018
//      1.) Fixed the start module ID issue
//      2.) Fixed the find all MIPS system when using ethernet ports
// 1.28, Oct 8,2018
//      1.) Fixed a bug in the DCbias menu that caused a crash when CH 9
//          or above were changed.
//      2.) Added message traffic to keep ethernet connections alive
// 1.29, Nov 1,2018
//      1.) Added the RFamp to the Custom Control Panel
// 1.30, Nov 17,2018
//      1.) Added the TCP/IP socket communications with control panels
//      2.) Added DAC channels to control panel
//      3.) Added ADC channels to control panel
//      4.) Fix RFamp shutdown to include DC volatges
//      5.) fixed control panel bug that caused crash on save methode
// 1.31, Nov 20, 2018
//      1.) Added new timing generation
//      2.) Fixed bugs in control panel
//      3.) refactored controlpanel code, a little bit, needs more work
// 1.32, Nov 27, 2018
//      1.) Fixed bug in RFamp that was not turning off DC supplies
//      2.) Added DC power supply enable to RFamp
//      3.) Save the DCoffsets first when saving methode file
// 1.33, Dec 2, 2018
//      1.) Added new timing generator to control panel
//      2.) Added TCP/IP updates to control panel
//      3.) Added external trigger options to FAIMS
// 1.34, Dec 13, 2018
//      1.) Fixed a number of bugs in the timing generator control
// 1.35, Dec 15, 2018
//      1.) Added Save, Load, and Clear all events buttons to timing generator
//      2.) Fixed bugs in timing generator
// 1.36, Dec 26, 2018
//      1.) Improved the control panel performance by using the read all DCbias
//          and RF driver commands.
//      2.) Addressed stability issues that were causing some system crashes.
//      3.) Added the properties page with ability to set folders and startup
//          behaivor.
// 1.37, Jan 6, 2019
//      1.) Added version to method file saved from control panel
//      2.) Added control panel file name to method file saved from control panel
//      3.) Changed signals from acquire program to be queued
//      4.) Disable trigger button until event is complete
//      5.) Added comm port re-open on timeout
// 1.38, Jan 7, 2019
//      1.) Added log file to properties and started adding log statements.
// 1.39, Jan 11, 2019
//      1.) Added logging of all MIPS status bar messages and control panel
//          status bar mesages
//      2.) Added delays in restore function in control panel
// 1.40, Jan 12, 2019
//      1.) Fixed a few minor bugs
//      2.) Updated logging, added MIPS FW version to control panel logs
//      3.) Added data log option
// 1.41, Jan 14, 2019
//      1.) Added serial critical error to logging
// 1.42, Jan 16, 2019
//      1.) Update the control panel restore function to set all voltages to 0
//          then turn on supplies and finally reset the voltages.
//      2.) Updated the script acquire function to abort is acquire is in progress.
//      3.) Added code to validate the data received from MIPS before populating the
//          text boxes.
// 1.43, Jan 19, 2019
//      1.) Fixed error in Compresssor dialog that causes the mode and switch radio
//          button to fail
// 1.44, Jan 22, 2019
//      1.) Removed all the lock and unlock commands, they caused all kinds of issues
// 1.45, Jan 24, 2019
//      1.) Removed the comms return if busy code
// 1.46, Jan 25, 2019
//      1.) Redesiged comms code so it blocks untill transaction is complete
//      2.) Added file path ti acquire dialog
// 1.47, Feb 1, 2019
//      1.) Added process events to the control panel update routine
//      2.) Changed process events interval to 10 mS in script processor
//      3.) Changed to non queued event processing in console window
// 1.48, Feb 2, 2019
//      1.) Remove lock code from control panel
//      2.) Removed queue option from connectors in timing classes
//      3.) Removed the use of the ringbuffer wait for line function
// 1.49, Feb 5, 2019
//      1.) Added wait for start to process start in cmdlineapp
//      2.) Found memory leak in the acquire class
//      3.) Fixed bug in ARB dir in the control panel code
// 1.50, Feb 14, 2019
//      1.) Fixed bug in the control panel arb waveform editor.
//      2.) Removed the doevents from the control panel update function, if it was
//          interrupted by the scripting engine then the update will freeze.
// 1.51, Feb 21, 2019
//      1.) Added update rates adjustment to properties page.
//      2.) Added MIPS box names to the methode file.
//      3.) Added wait for update function for scripting.
//      4.) Added MIPS name to comm errors
// 1.52, May 19, 2019
//      1.) Added uptime to the methode file save in control panel.
//      2.) Set RF drive to zero just before auto tune and then delay for 1 sec,
//          likely fixes a bug when pressing auto tune when at power.
//      3.) Fixed ARB edit, it depends of PPP for download. This is a temp fix
//          need to update the editor, fixed 03/11/19 but not tested
//      4.) Added the ability to popup a control panel from with in a control panel,
//          a child control panel. ControlPanel,Name,config file,X,Y
//      5.) Test for valid path when experiment starts and popup an error if invalid
//          path is detected.
//      6.) Added closed loop option to RF driver in control panel
//      7.) Added support for AMPS, the properties page allow you to enable searching
//          for AMPS system and allows you to select baud rate. AMPS are 8 bit, even
//          parity, 2 stop bits and Xon/Xoff flow control
//      8.) Added group box command to place controls in a group. This
//          will also required an end statement. Added group name string to
//          all controls supporting this function. This allows same name controls
//          in different groups.
//      9.) Added Script button to control panel. The button automatically calls a script
//          as defined in control panel config file.
//      10.)Added Plotting capability to control panel.
//      11.)Updated the timing generator to allow setting start and width times using other
//          event values, supports linking
// 1.53, June 18, 2019
//      1.) Updated the timing generator to support .Start and .Width selection fields
//      2.) Added rename to timing generator event editor
// 1.54, July 5, 2019
//      1.) Added Hadamard multiplexing capability to the pulse sequence generator, support
//          4, 5, and 6 bit
//      2.) Added plot capabilities to support het maps, for the FAIMSFB work
// 1.55, July 13, 2019
//      1.) Added the serial device capability to control panels
// 1.56, July 18, 2019
//      1.) Added Mux bits 7,8, and 9
//      2.) Added delay in sending chars to MIPS to support long tables.
// 1.57, July 23, 2019
//      1.) Added Socket command for the timing control, tigger, edit, and abort
//      2.) Fixed a bug in the delay added in very 1.56. I was delaying 10mS on every command.
// 1.58, August 6, 2019
//      1.) Fixed bug the multiplexing file generation
// 1.59, August 27, 2019
//      1.) Fixed a bugs in the control panel CSV functions
//      2.) Added command processing for script buttons
// 1.60, Sept 2, 2019
//      1.) Added -A option for Accumulations passing to acquire application
//      2.) Save and restore the timing generator Mux state
//
//      To dos for next release:
//          - Resolve table not downloaded issue
//          - Control panel to dos:
//              - Allow data file name definition, have U1084A.data be the default
// 1.61, Sept 25, 2019
//      1.) Added shutdown and restore to ARB channels in control panel
// 1.62, Oct 17, 2019
//      1.) Added time in mS mode to timing generator.
//      2.) Added external trigger mode support to timing generator.
//      3.) Fixed bug the DIO process command, found by Sunrise.
// 1.63, Oct 18, 2019
//      1.) Fixed but in the timing generation upgrades to time based operation.
// 1.64, Oct 24, 2019
//      1.) Fixed bug that caused acquire app finish to return to loc mode, in external
//          trigger mode we need to remain in table mode.
//      2.) Fixed bug that was intruduced in version 1.60, was not sending total tofs to acquire app
// 1.65, Oct 25, 2019
//      1.) Fixed the bug with enternal trigger in the control panel timing generator
//      2.) Added support for multiple timing generators in a control panel. The last timing generator
//          in the list is the only one that can use the acquire app.
//      3.) Updated the TCPserver new connection methode as per Harry at Sunrise.
//      4.) Updated the plot scripting functions.
//      5.) Fixed a bugs in the ploting function that would cause app to crash with invalid entries
// 1.66, Nov 1, 2019
//      1.) Added low level MIPS comms to the TCP/IP server
//      2.) Added EventControl to the control panel to allow editing table parameters on the fly
//      3.) Send external clock to MIPS in timing generator
//      4.) Added Savitzky-Golay filter to plot function
// 1.67, Nov 7, 2019
//      1.) Fixed bug in TCP server interface to timing control
//      2.) Fixed the event control to update the values in the timing control
// 1.68, Nov 10, 2019
//      1.) Added isTblMode to timing generator
//      2.) Changed the event control options to .Value and .ValueOff
// 1.69, Nov 18, 2019
//      1.) Updated the TCP server read routine to only remove the characters from the buffer that
//          will fit in the ring buffer. The characters that will not fit are left in the buffer.
// 1.70, Dec 13, 2019
//      1.) Updated RFamp interface to properly process enable and disable for pole bias and resolving DC bias
// 1.71, Jan 13, 2020
//      1.) Added the SendCommand function to the control panel to allow sending commands to MIPS from the control
//          pannel config file.
// 1.72, Jan 23, 2020
//      1.) Added CheckBox to custom control
//      2.) Added arrow key to value change to DAC control
//      3.) Allow control panel script button to be called on every update cycle
// 1.73, Feb 5, 2020
//      1.) Added commands to timing generator to return start time value and channel number
// 1.74, Feb 17, 2020
//      1.) Fix a bug in the control panel when loading settings files with overlapping names
//      2.) Added arrow value control to ARB control in the control panel
// 1.75, March 11, 2020
//      1.) Added backspace capability to console, this version sends the backspace char, 0x08
// 1.76, March 30, 2020
//      1.) added valitity testing to the mux timing generation
//      2.) fixed a sign bug in the timing generator
// 1.77, April 25, 2020
//      1.) Fixed a bug in the automatic serial port reconnection that only happened on a PC.
//      2.) Added SerialWatchDog support.
// 1.78, May 10, 2020
//      1.) Added the Shutter timing control to the control panel
// 1.79, May 14, 2020
//      1.) Redesigned the Shutter timing control, it have many bugs!
// 1.80, Sept 11, 2020
//      Updates for the 2nd generation softlanding systems
//      1.) Updated the ADC function to use the LOG output on gauges, also
//          has low threshold that will display OFF when gauge output is below limit.
//      2.) Disable all input of DIO input channels
//      3.) Allow DIO disable in control panel
//      4.) Fixed but in run script button
//      5.) Fixed bug in restoring RF quad enable state
//      6.) Added On exit script button
// 1.81, Dec 6, 2020
//      1.) Added time sweep function to shutter timing generator
// 1.82, Jan 26, 2021
//      1.) Updated the device serial opject to support 57600 baud
//      2.) Added .Update command to device serail object to support the spectrum script
// 1.83, Feb 4, 2021
//      1.) Update the acquire system to support an accquire app that keeps running and
//          allows you to restart.
// 1.84, Feb 28, 2021
//      1.) Updated FAIMS to include auto tune options and curtain supply control.
// 1.85, May 2, 2021
//      1.) Added auto reconnect to the app when not running control panel
//      2.) Read the version from MIPS and save it in comms object major.minor
//      3.) Updated the FAIMS tab to read use the MIPS version to enable and
//          disable options.
//      4.) Added options for negative peak tune, and curtian supply options
//
// Planded updates:
//      - Add ploting capability. Also support this through the Scripting system.
//
#include "mips.h"
#include "ui_mips.h"
#include "console.h"
#include "settingsdialog.h"
#include "pse.h"
#include "ringbuffer.h"
#include "comms.h"
#include "twave.h"
#include "dcbias.h"
#include "dio.h"
#include "rfdriver.h"
#include "psg.h"
#include "program.h"
#include "help.h"
#include "arb.h"
#include "faims.h"
#include "filament.h"
#include "singlefunnel.h"
#include "softlanding.h"
#include "softlanding2.h"
#include "grid.h"
#include "arbwaveformedit.h"
#include "adc.h"
#include "controlpanel.h"
#include "scriptingconsole.h"
#include "tcpserver.h"
#include "properties.h"

#include <QMessageBox>
#include <QtSerialPort/QSerialPort>
#include <QTime>
#include <QThread>
#include <QIntValidator>
#include <QDoubleValidator>
#include <QCursor>
#include <QDebug>
#include <QFileDialog>
#include <QDir>
#include <QProcess>
#include <QFileInfo>
#include <QtNetwork/QTcpSocket>
#include <QInputDialog>

QString Version = "MIPS, Version 1.85 May 2, 2021";

MIPS::MIPS(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MIPS)
{

//    qDebug() << "hi";
//    float fval;
//    std::sscanf("&S=,Range=002nA,-0.0378,nA", "%*16c%f",&fval);
//    qDebug() << fval;

    ui->setupUi(this);
    ui->tabMIPS->setElideMode(Qt::ElideNone);
    ui->tabMIPS->setUsesScrollButtons(true);
    // Make the dialog fixed size.
    this->setFixedSize(this->size());

    ui->comboSystems->setVisible(false);
    ui->lblSystems->setVisible(false);

    MIPS::setWindowTitle(Version);

    #if defined(Q_OS_MAC)
    QFont font = ui->lblMIPSconnectionNotes->font();
    font.setPointSize(15);
    ui->lblMIPSconnectionNotes->setFont(font);
    #endif

    properties = new Properties;
    scriptconsole = NULL;
    qApp->installEventFilter(this);
    sf = NULL;
    sf_deleteRequest = false;
    sl = NULL;
    sl_deleteRequest = false;
    sl2 = NULL;
    sl2_deleteRequest = false;
    grid = NULL;
    grid_deleteRequest = false;
    cp = NULL;
    cp_deleteRequest = false;
    appPath = QApplication::applicationDirPath();
    pollTimer = new QTimer;
    settings = new SettingsDialog;
    comms  = new Comms(settings,"",ui->statusBar);
    console = new Console(ui->Terminal,ui,comms);
    console->setEnabled(false);
    twave  = new Twave(ui,comms);
    dcbias = new DCbias(ui,comms);
    dio = new DIO(ui,comms);
    rfdriver = new RFdriver(ui,comms);
    SeqGen = new PSG(ui,comms);
    pgm = new Program(ui, comms, console);
    help = new Help();
    arb = new ARB(ui, comms);
    faims = new FAIMS(ui, comms);
    filament = new Filament(ui, comms);
    adc = new ADC(ui, comms);

    RepeatMessage = "";
    ui->actionClear->setEnabled(true);
    ui->actionOpen->setEnabled(true);
    ui->actionSave->setEnabled(true);
    ui->actionHelp->setEnabled(true);
    ui->actionMIPS_commands->setEnabled(true);
    ui->actionProgram_MIPS->setEnabled(true);
    ui->actionSave_current_MIPS_firmware->setEnabled(true);
    ui->actionSet_bootloader_boot_flag->setEnabled(true);
    ui->actionSingle_Funnel->setEnabled(true);
    ui->actionSoft_landing->setEnabled(true);
    ui->actionSoft_landing_Purdue->setEnabled(true);
    ui->actionGrid->setEnabled(true);
    ui->actionMessage_repeat->setEnabled(true);
    ui->actionGet_file_from_MIPS->setEnabled(true);
    ui->actionSend_file_to_MIPS->setEnabled(true);
    ui->actionRead_EEPROM->setEnabled(true);
    ui->actionWrite_EEPROM->setEnabled(true);
    ui->actionRead_ARB_FLASH->setEnabled(true);
    ui->actionWrite_ARB_FLASH->setEnabled(true);
    ui->actionARB_upload->setEnabled(true);
    ui->actionSelect->setEnabled(true);
    ui->menuTerminal->setEnabled(false);
    ui->actionScripting->setEnabled(true);

    connect(ui->actionProperties,SIGNAL(triggered(bool)), this, SLOT(DisplayProperties()));
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(loadSettings()));
    connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(saveSettings()));
    connect(ui->pbConfigure, SIGNAL(pressed()), settings, SLOT(show()));
    connect(ui->actionMIPS_commands, SIGNAL(triggered()), this, SLOT(MIPScommands()));
    connect(ui->actionHelp, SIGNAL(triggered()), this, SLOT(GeneralHelp()));
    connect(ui->tabMIPS,SIGNAL(currentChanged(int)),this,SLOT(tabSelected()));
    connect(ui->pbConnect,SIGNAL(pressed()),this,SLOT(MIPSconnect()));
    connect(ui->pbSearchandConnect,SIGNAL(pressed()),this,SLOT(MIPSsearch()));
    connect(ui->pbDisconnect,SIGNAL(pressed()),this,SLOT(MIPSdisconnect()));
    connect(pollTimer, SIGNAL(timeout()), this, SLOT(pollLoop()));
    connect(ui->actionAbout,SIGNAL(triggered(bool)), this, SLOT(DisplayAboutMessage()));
    connect(ui->actionClear, SIGNAL(triggered()), this, SLOT(clearConsole()));
    connect(ui->actionSingle_Funnel, SIGNAL(triggered()), this, SLOT(Single_Funnel()));
    connect(ui->actionSoft_landing, SIGNAL(triggered()), this, SLOT(Soft_Landing()));
    connect(ui->actionSoft_landing_Purdue, SIGNAL(triggered()), this, SLOT(Soft_Landing_Purdue()));
    connect(ui->actionGrid, SIGNAL(triggered()), this, SLOT(Grid_system()));
    connect(ui->actionMessage_repeat, SIGNAL(triggered()), this, SLOT(GetRepeatMessage()));
    connect(ui->actionGet_file_from_MIPS, SIGNAL(triggered()), this, SLOT(GetFileFromMIPS()));
    connect(ui->actionSend_file_to_MIPS, SIGNAL(triggered()), this, SLOT(PutFiletoMIPS()));
    connect(ui->actionRead_EEPROM, SIGNAL(triggered()), this, SLOT(ReadEEPROM()));
    connect(ui->actionWrite_EEPROM, SIGNAL(triggered()), this, SLOT(WriteEEPROM()));
    connect(ui->actionRead_ARB_FLASH, SIGNAL(triggered()), this, SLOT(ReadARBFLASH()));
    connect(ui->actionWrite_ARB_FLASH, SIGNAL(triggered()), this, SLOT(WriteARBFLASH()));
    connect(ui->actionARB_upload, SIGNAL(triggered()), this, SLOT(ARBupload()));
    connect(ui->actionSelect, SIGNAL(triggered()), this, SLOT(SelectCP()));
    connect(ui->actionScripting, SIGNAL(triggered()), this, SLOT(slotScripting()));
    connect(ui->statusBar,SIGNAL(messageChanged(QString)),this,SLOT(slotLogStatusBarMessage(QString)));

    ui->comboMIPSnetNames->installEventFilter(new DeleteHighlightedItemWhenShiftDelPressedEventFilter);
    // Sets the polling loop interval and starts the timer
    if(properties != NULL) pollTimer->start(1000 * properties->UpdateSecs);
    else pollTimer->start(1000);

    for(int i=0;i<properties->MIPS_TCPIP.count();i++) ui->comboMIPSnetNames->addItem(properties->MIPS_TCPIP[i]);
    if(properties != NULL) properties->Log("MIPS loaded: " + Version);
}

MIPS::~MIPS()
{
//    if(sl != NULL) delete sl;
//    if(sf != NULL) delete sf;
//    delete settings;
//    delete ui;
    if(properties != NULL) properties->Log("MIPS closing: " + Version);
}

void MIPS::closeEvent(QCloseEvent *event)
{
    if(sl != NULL) delete sl;
    if(sl2 != NULL) delete sl2;
    if(sf != NULL) delete sf;
    if(grid != NULL) delete grid;
    if(cp != NULL) delete cp;
    delete settings;
    delete help;
    delete ui;
}

bool MIPS::eventFilter(QObject *obj, QEvent *event)
{
    if(SelectedTab == "DCbias") return dcbias->myEventFilter(obj, event);
    if(SelectedTab == "Twave") return twave->myEventFilter(obj, event);
  return QObject::eventFilter(obj, event);
}

bool DeleteHighlightedItemWhenShiftDelPressedEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        //qDebug() << keyEvent->key() << Qt::Key::Key_Delete <<Qt::Key::Key_Backspace;
        if ((keyEvent->key() == Qt::Key::Key_Backspace) && (keyEvent->modifiers() == Qt::ShiftModifier))
        {
            //qDebug() << "filter";
            auto combobox = dynamic_cast<QComboBox *>(obj);
            if (combobox){
                combobox->removeItem(combobox->currentIndex());
                return true;
            }
        }
    }
    // standard event processing
    return QObject::eventFilter(obj, event);
}

void MIPS::MIPScommands(void)
{
    help->SetTitle("MIPS Commands");
    help->LoadHelpText(":/MIPScommands.txt");
    help->show();
}

// Removes a tab by its name
void MIPS::RemoveTab(QString TabName)
{
   for(int i = 0; i < ui->tabMIPS->count(); i++)
   {
       if( ui->tabMIPS->tabText(i) == TabName)
       {
           ui->tabMIPS->removeTab(i);
           return;
       }
   }
}

void MIPS::AddTab(QString TabName)
{
    // If tab is present then do nothing and exit
    for(int i = 0; i < ui->tabMIPS->count(); i++)
    {
        if( ui->tabMIPS->tabText(i) == TabName)
        {
            return;
        }
    }
    if(TabName == "DCbias") ui->tabMIPS->insertTab(3,ui->DCbias, "DCbias");
    if(TabName == "RFdriver") ui->tabMIPS->insertTab(3,ui->RFdriver, "RFdriver");
    if(TabName == "Twave") ui->tabMIPS->insertTab(3,ui->Twave, "Twave");
    if(TabName == "ARB") ui->tabMIPS->insertTab(3,ui->ARB, "ARB");
    if(TabName == "FAIMS") ui->tabMIPS->insertTab(3,ui->FAIMS, "FAIMS");
    if(TabName == "Filament") ui->tabMIPS->insertTab(3,ui->Filament, "Filament");
}

void MIPS::GeneralHelp(void)
{
    help->SetTitle("MIPS Help");
    help->LoadHelpText(":/MIPShelp.txt");
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "System")
    {
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
    {
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Digital IO")
    {
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "DCbias")
    {
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "RFdriver")
    {
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "FAIMS")
    {
        help->SetTitle("FAIMS help");
        help->LoadHelpText(":/FAIMShelp.txt");
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Pulse Sequence Generation")
    {
        help->SetTitle("Pulse Sequence Generation Help");
        help->LoadHelpText(":/PSGhelp.txt");
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "ARB")
    {
        help->SetTitle("ARB Help");
        help->LoadHelpText(":/ARBhelp.txt");
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Twave")
    {
        help->SetTitle("Twave Help");
        help->LoadHelpText(":/TWAVEhelp.txt");
    }
    if(sl != NULL)
    {
        help->SetTitle("Soft Landing Help");
        help->LoadHelpText(":/SoftLandingHelp.txt");
    }
    if(sl2 != NULL)
    {
        help->SetTitle("Soft Landing Help");
        help->LoadHelpText(":/SoftLandingHelp2.txt");
    }
    if(sf != NULL)
    {
        help->SetTitle("Single Funnel Help");
        help->LoadHelpText(":/SingleFunnelHelp.txt");
    }
    help->show();
    return;
}

void MIPS::DisplayAboutMessage(void)
{
    QMessageBox::information(
        this,
        tr("Application Named"),
        tr("MIPS interface application, written by Gordon Anderson. This application allows communications with the MIPS system supporting monitoring and control as well as pulse sequence generation.") );
}

void MIPS::DisplayProperties(void)
{
    properties->exec();
}

void MIPS::loadSettings(void)
{
    if(sl != NULL)
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Settings (*.settings);;All files (*.*)"));
        sl->Load(fileName);
        return;
    }
    if(sl2 != NULL)
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Settings (*.settings);;All files (*.*)"));
        sl2->Load(fileName);
        return;
    }
    if(sf != NULL)
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Settings (*.settings);;All files (*.*)"));
        sf->Load(fileName);
        return;
    }
    if(grid != NULL)
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Settings (*.settings);;All files (*.*)"));
        grid->Load(fileName);
        return;
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "System")
    {
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Data from File"),"",tr("Settings (*.settings);;All files (*.*)"));
        console->Load(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Digital IO")
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Settings (*.settings);;All files (*.*)"));
        dio->Load(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "DCbias")
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Settings (*.settings);;All files (*.*)"));
        dcbias->Load(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "RFdriver")
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Settings (*.settings);;All files (*.*)"));
        rfdriver->Load(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "FAIMS")
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Settings (*.settings);;All files (*.*)"));
        faims->Load(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Pulse Sequence Generation")
    {
        SeqGen->Load();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Twave")
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Settings (*.settings);;All files (*.*)"));
        twave->Load(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "ARB")
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load Settings from File"),"",tr("Settings (*.settings);;All files (*.*)"));
        arb->Load(fileName);
    }
    return;
}

void MIPS::saveSettings(void)
{
    if(sl != NULL)
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Settings (*.settings);;All files (*.*)"));
        sl->Save(fileName);
        return;
    }
    if(sl2 != NULL)
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Settings (*.settings);;All files (*.*)"));
        sl2->Save(fileName);
        return;
    }
    if(sf != NULL)
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Settings (*.settings);;All files (*.*)"));
        sf->Save(fileName);
        return;
    }
    if(grid != NULL)
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Settings (*.settings);;All files (*.*)"));
        grid->Save(fileName);
        return;
    }
    if(cp != NULL)
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Settings (*.settings);;All files (*.*)"));
        cp->Save(fileName);
        return;
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "System")
    {
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Data File"),"",tr("Settings (*.settings);;All files (*.*)"));
        console->Save(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Digital IO")
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Settings (*.settings);;All files (*.*)"));
        dio->Save(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "DCbias")
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Settings (*.settings);;All files (*.*)"));
        dcbias->Save(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "RFdriver")
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Settings (*.settings);;All files (*.*)"));
        rfdriver->Save(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "FAIMS")
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Settings (*.settings);;All files (*.*)"));
        faims->Save(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Pulse Sequence Generation")
    {
        SeqGen->Save();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Twave")
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Settings (*.settings);;All files (*.*)"));
        twave->Save(fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "ARB")
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to Settings File"),"",tr("Settings (*.settings);;All files (*.*)"));
        arb->Save(fileName);
    }
    return;
}

void MIPS::pollLoop(void)
{
    static  bool firstCall = true;
    QString res ="";

    if(firstCall)
    {
        if(properties->AutoConnect) MIPSsearch();
        if(Systems.count() >= properties->MinMIPS)
        {
           if(properties->LoadControlPanel != "") SelectCP(properties->LoadControlPanel);
        }
    }
    firstCall = false;
    if(scriptconsole!=NULL) scriptconsole->UpdateStatus();
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Pulse Sequence Generation")
    {
        /*
        while(true)
        {
            c = rb.getch();
            if((int)c==0) return;
            ui->statusBar->showMessage(ui->statusBar->currentMessage() + c,2000);
        }
        */
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
    {
        if(RepeatMessage != "") comms->SendString(RepeatMessage.toStdString().c_str());
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "DCbias")
    {
         if(ui->chkDCBautoUpdate->isChecked())  dcbias->Update();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "RFdriver")
    {
        //UpdateRFdriver();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Filament")
    {
        if(comms->isConnected()) filament->Update();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "ARB")
    {
        if(comms->isConnected()) if(ui->chkARBautoUpdate->isChecked()) arb->Update();
    }
    if(comms->isConnected()) faims->PollLoop();
    if(sf_deleteRequest)
    {
      delete sf;
      sf = NULL;
      sf_deleteRequest = false;
    }
    if(sf != NULL) if(comms->isConnected()) sf->Update();

    if(sl_deleteRequest)
    {
      delete sl;
      sl = NULL;
      sl_deleteRequest = false;
    }
    if(sl != NULL) if(comms->isConnected()) sl->Update();

    if(sl2_deleteRequest)
    {
      delete sl2;
      sl2 = NULL;
      sl2_deleteRequest = false;
    }
    if(sl2 != NULL) if(comms->isConnected()) sl2->Update();

    if(grid_deleteRequest)
    {
      delete grid;
      grid = NULL;
      grid_deleteRequest = false;
    }
    if(grid != NULL) if(comms->isConnected()) grid->Update();

    if(cp_deleteRequest)
    {
      delete cp;
      cp = NULL;
      cp_deleteRequest = false;
    }
    //if(cp != NULL) if(comms->isConnected()) cp->Update();
    if(cp != NULL) cp->Update();
 }

void MIPS::mousePressEvent(QMouseEvent * event)
{
    QMainWindow::mousePressEvent(event);
}

void MIPS::resizeEvent(QResizeEvent* event)
{

   ui->tabMIPS->setFixedWidth(MIPS::width());
   #if defined(Q_OS_MAC)
    ui->tabMIPS->setFixedHeight(MIPS::height()-(ui->statusBar->height()));
   #else
    // Not sure why I need this 3x for a windows system??
    ui->tabMIPS->setFixedHeight(MIPS::height()-(ui->statusBar->height()*3));
   #endif

   console->resize(ui->Terminal);
   QMainWindow::resizeEvent(event);
}

void delay()
{
    QTime dieTime= QTime::currentTime().addSecs(1);
    while (QTime::currentTime() < dieTime)
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void MIPS::MIPSsetup(void)
{
    QString res;
    int numRF=0,numDCB=0;

    pgm->comms = comms;
    ui->lblMIPSconfig->setText("MIPS: ");
    // Turn ECHO off
    comms->SendString("\n");
    comms->SendCommand("ECHO,FALSE\n");
    res = comms->SendMess("GVER\n");
    if(res=="") return;  // Exit if we timeout, no MIPS comms
    ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + res);
    ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\nModules present:");

    res = comms->SendMess("GCHAN,RF\n");
    if(res=="") return;
    if(res.contains("2")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   1 RF driver\n");
    if(res.contains("4")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   2 RF drivers\n");
    numRF = res.toInt();
    if(res.contains("0")) RemoveTab("RFdriver");
    else AddTab("RFdriver");

    res = comms->SendMess("GCHAN,DCB\n");
    if(res=="") return;
    if(res.contains("8")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text()  + "\n   1 DC bias (8 output channels)\n");
    if(res.contains("16")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   2 DC bias (16 output channels)\n");
    if(res.contains("24")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   3 DC bias (16 output channels)\n");
    if(res.contains("32")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   4 DC bias (16 output channels)\n");
    numDCB = res.toInt();
    if(res.contains("0")) RemoveTab("DCbias");
    else AddTab("DCbias");

    res = comms->SendMess("GCHAN,TWAVE\n");
    if(res=="") return;
    if(res.contains("1")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   1 TWAVE module\n");
    if(res.contains("2")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   2 TWAVE modules\n");
    if(res.contains("0")) RemoveTab("Twave");
    else AddTab("Twave");

    res = comms->SendMess("GCHAN,ARB\n");
    if(res=="") return;
    if(res.contains("8")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() +  "\n   1 ARB module\n");
    if(res.contains("16")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   2 ARB modules\n");
    if(res.contains("24")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   3 ARB modules\n");
    if(res.contains("32")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   4 ARB modules\n");
    if(res.contains("0")) RemoveTab("ARB");
    else AddTab("ARB");
    arb->SetNumberOfChannels(res.toInt());

    res = comms->SendMess("GCHAN,FAIMS\n");
    if(res=="") return;
    if(res.contains("1")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   FAIMS module\n");
    if(res.contains("0")) RemoveTab("FAIMS");
    else AddTab("FAIMS");

    res = comms->SendMess("GCHAN,ESI\n");
    if(res=="") return;
    if(res.contains("1")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   1 ESI module, rev 3\n");
    if(res.contains("2")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   1 ESI module\n");
    if(res.contains("4")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   2 ESI modules\n");

    res = comms->SendMess("GCHAN,FIL\n");
    if(res=="") return;
    if(res.contains("1")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   1 Filament module\n");
    if(res.contains("2")) ui->lblMIPSconfig->setText(ui->lblMIPSconfig->text() + "\n   2 Filament modules\n");
    if(res.contains("0")) RemoveTab("Filament");
    else AddTab("Filament");

    rfdriver->SetNumberOfChannels(numRF);
    dcbias->SetNumberOfChannels(numDCB);

}

// Here when the Connect push button is pressed. This function makes a connection with MIPS.
// If a network name or IP is provided that connection is tried first.
// If the serial port of socket is connected then this function exits.
void MIPS::MIPSconnect(void)
{
    comms->p = settings->settings();
    comms->properties = properties;
    comms->host = ui->comboMIPSnetNames->currentText();
    if(comms->ConnectToMIPS())
    {
       Systems.clear();
       Systems << comms;
       console->setEnabled(true);
       console->setLocalEchoEnabled(settings->settings().localEchoEnabled);
       ui->lblMIPSconnectionNotes->setHidden(true);
       MIPSsetup();
    }
}

void MIPS::MIPSsearch(void)
{
    QMessageBox *msg = new QMessageBox();
    msg->setText("Searching for MIPS system...");
    msg->setStandardButtons(0);
    msg->show();
    QApplication::processEvents();
    QApplication::processEvents();
    QApplication::processEvents();

    settings->fillPortsParameters();
    settings->fillPortsInfo();
    FindMIPSandConnect();
    msg->hide();
}

void MIPS::FindAllMIPSsystems(void)
{
    Comms *cp;

    disconnect(ui->comboSystems, SIGNAL(currentIndexChanged(int)), 0, 0);
    Systems.clear();
    // If there are a defined list of net names or IP addresses
    // then use them and do not search for USB connected systems
    if(ui->comboMIPSnetNames->count() > 0)
    {
        for(int j=0;j<ui->comboMIPSnetNames->count();j++)
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
            cp = new Comms(settings,"",ui->statusBar);
            cp->host = ui->comboMIPSnetNames->itemText(j);
            // if(cp->isMIPS(settings->getPortName(j)))    //Not sure why this was here?
            if(cp->ConnectToMIPS())
            {
               Systems << (cp);
               cp = new Comms(settings,"",ui->statusBar);
            }
        }
    }
    else
    {
        int i = settings->numberOfPorts();
        settings->fillPortsInfo();
        for(int j=0;j<i;j++)
        {
          ui->statusBar->showMessage("Trying: " + settings->getPortName(j));
          QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
          cp = new Comms(settings,"",ui->statusBar);
          cp->properties = properties;
          if(cp->isMIPS(settings->getPortName(j)))
          {
            delay();
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
            cp->host = "";
            cp->SendString("ECHO,FALSE\n");
            if(cp->ConnectToMIPS())
            {
               Systems << (cp);
               cp = new Comms(settings,"",ui->statusBar);    // Why??
            }
          }
          else if(properties != NULL)
          {
              if(properties->SearchAMPS)
              {
                  if(cp->isAMPS(settings->getPortName(j),properties->AMPSbaud))
                  {
                    delay();
                    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
                    cp->host = "";
                    //cp->SendString("RTM,ON\n");  // Port is closed here
                    if(cp->ConnectToMIPS())
                    {
                       Systems << (cp);
                       cp = new Comms(settings,"",ui->statusBar);  // Why??
                    }
                  }
              }
          }
        }
    }
    ui->comboSystems->clear();
    for(int j=0;j<Systems.count();j++)
    {
        ui->comboSystems->addItem(Systems.at(j)->MIPSname);
    }
    if(ui->comboSystems->count() > 1)
    {
        ui->comboSystems->setVisible(true);
        ui->lblSystems->setVisible(true);
    }
}

void MIPS::FindMIPSandConnect(void)
{
  ui->pbSearchandConnect->setDown(false);
  QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
  FindAllMIPSsystems();
  if(ui->comboSystems->count() > 0)
  {
      delay();
      comms = Systems.at(0);
      console->setEnabled(true);
      console->setLocalEchoEnabled(settings->settings().localEchoEnabled);
      ui->lblMIPSconnectionNotes->setHidden(true);
      MIPSsetup();
      connect(ui->comboSystems, SIGNAL(currentIndexChanged(int)), this, SLOT(UpdateSystem()));
  }
  else ui->statusBar->showMessage(tr("Can't find MIPS system!"));
  return;

  int i = settings->numberOfPorts();
  for(int j=0;j<i;j++)
  {
    if(comms->isMIPS(settings->getPortName(j)))
    {
      delay();
      comms->host = ui->comboSystems->currentText();
      if(comms->ConnectToMIPS())
      {
        console->setEnabled(true);
        console->setLocalEchoEnabled(settings->settings().localEchoEnabled);
        ui->lblMIPSconnectionNotes->setHidden(true);
        MIPSsetup();
      }
      return;
    }
  }
  ui->statusBar->showMessage(tr("Can't find MIPS system!"));
}

void MIPS::MIPSdisconnect(void)
{
    comms->DisconnectFromMIPS();
    for(int j=0;j<Systems.count();j++)
    {
        Systems.at(j)->DisconnectFromMIPS();
    }
//    ui->comboSystems->clear();
    ui->comboSystems->setVisible(false);
    ui->lblSystems->setVisible(false);

    ui->lblMIPSconfig->setText("");
    ui->lblMIPSconnectionNotes->setHidden(false);
//    qApp->quit();
//    QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
}

void MIPS::UpdateSystem(void)
{
   if(Systems.count() == 0) return;
   comms = Systems.at(ui->comboSystems->currentIndex());
   MIPSsetup();
}

void MIPS::tabSelected()
{
    static QString LastTab = "System";
    if(properties != NULL) properties->Log("Tab selected, " + ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()));
    disconnect(comms, SIGNAL(DataReady()),0,0);
    disconnect(console, SIGNAL(getData(QByteArray)),0,0);
    ui->menuTerminal->setEnabled(false);
    if(LastTab == "Pulse Sequence Generation")
    {
        // Abort when exiting this tab
        comms->SendString("TBLABRT\n");
        delay();
        comms->rb.clear();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "System")
    {
        LastTab = "System";
        settings->fillPortsInfo();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "ADC")
    {
        LastTab = "ADC";
        adc->Update(true);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
    {
        ui->menuTerminal->setEnabled(true);
        LastTab = "Terminal";
        connect(comms, SIGNAL(DataReady()), this, SLOT(readData2Console()));
        connect(console, SIGNAL(getData(QByteArray)), this, SLOT(writeData(QByteArray)));
        console->resize(ui->Terminal);
        console->setEnabled(true);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Digital IO")
    {
        ui->chkRemoteNav->setChecked(false);
        dio->comms = comms;
        LastTab = "Digital IO";
        dio->Update();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "DCbias")
    {
        dcbias->comms = comms;
        LastTab = "DCbias";
        dcbias->Update();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "RFdriver")
    {
        rfdriver->comms = comms;
        LastTab = "RFdriver";
        rfdriver->Update();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Pulse Sequence Generation")
    {
        SeqGen->comms = comms;
        LastTab = "Pulse Sequence Generation";
        SeqGen->Update();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Twave")
    {
        twave->comms = comms;
        LastTab = "Twave";
        twave->Update();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "ARB")
    {
        arb->comms = comms;
        LastTab = "ARB";
        arb->Update();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "FAIMS")
    {
        faims->comms = comms;
        LastTab = "FAIMS";
        faims->Update();
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Filament")
    {
        filament->comms = comms;
        LastTab = "Filament";
        filament->Update();
    }
    SelectedTab = LastTab;
}

void MIPS::writeData(const QByteArray &data)
{
    comms->writeData(data);
}

void MIPS::readData2Console(void)
{
    QByteArray data;

    data = comms->readall();
    console->putData(data);
}

void MIPS::CloseSingleFunnel(void)
{
   sf_deleteRequest = true;
}

void MIPS::Single_Funnel(void)
{
   if(sf != NULL) return;
   if(sl != NULL) return;
   if(sl2 != NULL) return;
   if(grid != NULL) return;
   if(cp != NULL) return;
   ui->tabMIPS->setCurrentIndex(0);
   sf = new SingleFunnel(0,comms,ui->statusBar);
   if(!sf->ConfigurationCheck())
   {
       delete sf;
       sf = NULL;
       return;
   }
   sf->show();
   connect(sf, SIGNAL(DialogClosed()), this, SLOT(CloseSingleFunnel()));
   sf->Update();
}

void MIPS::CloseSoftLanding(void)
{
   sl_deleteRequest = true;
}

void MIPS::Soft_Landing(void)
{
    if(sf != NULL) return;
    if(sl != NULL) return;
    if(sl2 != NULL) return;
    if(grid != NULL) return;
    if(cp != NULL) return;
    ui->tabMIPS->setCurrentIndex(0);
    sl = new SoftLanding(0,comms,ui->statusBar);
    if(!sl->ConfigurationCheck())
    {
        delete sl;
        sl = NULL;
        return;
    }
    sl->show();
    connect(sl, SIGNAL(DialogClosed()), this, SLOT(CloseSoftLanding()));
    sl->Update();
}

void MIPS::CloseSoftLanding2(void)
{
   sl2_deleteRequest = true;
}

void MIPS::Soft_Landing_Purdue(void)
{
    if(sf != NULL) return;
    if(sl != NULL) return;
    if(sl2 != NULL) return;
    if(grid != NULL) return;
    if(cp != NULL) return;
    ui->tabMIPS->setCurrentIndex(0);
    sl2 = new SoftLanding2(0,comms,ui->statusBar);
    if(!sl2->ConfigurationCheck())
    {
        delete sl2;
        sl2 = NULL;
        return;
    }
    sl2->show();
    connect(sl2, SIGNAL(DialogClosed()), this, SLOT(CloseSoftLanding2()));
    sl2->Update();
}

void MIPS::CloseGridSystem(void)
{
   grid_deleteRequest = true;
}

void MIPS::Grid_system(void)
{
    if(sf != NULL) return;
    if(sl != NULL) return;
    if(sl2 != NULL) return;
    if(grid != NULL) return;
    if(cp != NULL) return;
    ui->tabMIPS->setCurrentIndex(0);
    grid = new Grid(0,comms,ui->statusBar);
    grid->show();
    connect(grid, SIGNAL(DialogClosed()), this, SLOT(CloseGridSystem()));
    grid->Update();
}

void MIPS::setWidgets(QWidget* old, QWidget* now)
{
}

void MIPS::clearConsole(void)
{
    console->clear();
}

void MIPS::GetRepeatMessage(void)
{
    bool ok;

    QString text = QInputDialog::getText(0, "MIPS", "Enter Command to send every sec:", QLineEdit::Normal,"", &ok );
    if ( ok && !text.isEmpty() )
    {
        RepeatMessage =  text + "\n";
    } else
    {
        RepeatMessage = "";
    }
}

void MIPS::GetFileFromMIPS(void)
{
    bool ok;

    // Get file name from user
    QString text = QInputDialog::getText(0, "MIPS", "File name to copy from MIPS SD card:", QLineEdit::Normal,"", &ok );
    if ( ok && !text.isEmpty() )
    {
        // Let user pick detination location and file name
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to File"),"",tr("All files (*.*)"));
        if(fileName != "")
        {
            // If on terminal tab then turn off the slot for received chars
            if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal") disconnect(comms, SIGNAL(DataReady()),0,0);
            comms->GetMIPSfile(text,fileName);
            if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
            {
                connect(comms, SIGNAL(DataReady()), this, SLOT(readData2Console()));
                readData2Console();
            }
        }
     }
}

void MIPS::PutFiletoMIPS(void)
{
    bool ok;

    // Let user pick source location and file name
    QString fileName = QFileDialog::getOpenFileName(this, tr("File to MIPS"),tr("All files (*)", 0));
    if(fileName == "") return;
    // If on terminal tab then turn off the slot for received chars
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal") disconnect(comms, SIGNAL(DataReady()),0,0);
    // Get file name from user
    QString text = QInputDialog::getText(0, "MIPS", "File name to copy to MIPS SD card:", QLineEdit::Normal,"", &ok );
    if ( ok && !text.isEmpty() )
    {
        comms->PutMIPSfile(text,fileName);
    }
    if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
    {
        connect(comms, SIGNAL(DataReady()), this, SLOT(readData2Console()));
        readData2Console();
    }
}

void MIPS::ReadEEPROM(void)
{
    bool ok;

    QMessageBox msgbox;
    msgbox.setText("MIPS Module EEPROM read function");
    msgbox.setInformativeText("Select the module A or B:");
    msgbox.addButton(tr("B"), QMessageBox::NoRole);
    msgbox.addButton(tr("A"), QMessageBox::NoRole);
    msgbox.addButton(tr("Cancel"), QMessageBox::NoRole);
    int brd = msgbox.exec();
    if(brd == 2) return;
    QString Address = QInputDialog::getText(this, "Read MIPS EEPROM", "Enter EEPROM address in hex:", QLineEdit::Normal,QString::null, &ok);
    if(ok && !Address.isEmpty())
    {
        // Here with the EEPROM module address and physical address
        // Selet a file to save the EEPROM contents
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save to File"),"",tr("All files (*.*)"));
        if(fileName != "")
        {
            // Read the data from MIPS
            if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal") disconnect(comms, SIGNAL(DataReady()),0,0);
            if(brd == 1) comms->GetEEPROM(fileName,"A",Address.toInt(0,16));
            else comms->GetEEPROM(fileName,"B",Address.toInt(0,16));
            if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
            {
                connect(comms, SIGNAL(DataReady()), this, SLOT(readData2Console()));
                readData2Console();
            }
        }
    }
}

void MIPS::WriteEEPROM(void)
{
    bool ok;

    QMessageBox msgbox;
    msgbox.setText("MIPS Module EEPROM write function. This function will allow you to reprogram the configuration memory of the selected module. Proceed with caution, you can render your system inoperable by entering invalid information.");
    msgbox.setInformativeText("Select the module A or B or cancel:");
    msgbox.addButton(tr("B"), QMessageBox::NoRole);
    msgbox.addButton(tr("A"), QMessageBox::NoRole);
    msgbox.addButton(tr("Cancel"), QMessageBox::NoRole);
    int brd = msgbox.exec();
    if(brd == 2) return;
    QString Address = QInputDialog::getText(this, "Write MIPS EEPROM", "Enter EEPROM address in hex:", QLineEdit::Normal,QString::null, &ok);
    if(ok && !Address.isEmpty())
    {
        // Here with the EEPROM module address and physical address
        // Selet a file to write to the EEPROM
        QString fileName = QFileDialog::getOpenFileName(this, tr("File to MIPS"),tr("All files (*)", 0));
        if(fileName != "")
        {
            // Read the data from MIPS
            if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal") disconnect(comms, SIGNAL(DataReady()),0,0);
            if(brd == 1) comms->PutEEPROM(fileName,"A",Address.toInt(0,16));
            else comms->PutEEPROM(fileName,"B",Address.toInt(0,16));
            if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
            {
                connect(comms, SIGNAL(DataReady()), this, SLOT(readData2Console()));
                readData2Console();
            }
        }
    }
}

void MIPS::ReadARBFLASH(void)
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save to File"),"",tr("All files (*.*)"));
    if(fileName != "")
    {
        // Read the data from MIPS
        if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal") disconnect(comms, SIGNAL(DataReady()),0,0);
        comms->GetARBFLASH(fileName);
        if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
        {
            connect(comms, SIGNAL(DataReady()), this, SLOT(readData2Console()));
            readData2Console();
        }
    }
}

void MIPS::WriteARBFLASH(void)
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("File to MIPS"),tr("All files (*)", 0));
    if(fileName != "")
    {
        // Read the data from MIPS
        if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal") disconnect(comms, SIGNAL(DataReady()),0,0);
        comms->PutARBFLASH(fileName);
        if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
        {
            connect(comms, SIGNAL(DataReady()), this, SLOT(readData2Console()));
            readData2Console();
        }
    }
}

void MIPS::ARBupload(void)
{
    bool ok;

    QString Faddress = QInputDialog::getText(0, "ARB upload", "ARB Module FLASH write function. This function will allow\nyou to upload a file and place it in FLASH at the address\nyou select. Proceed with caution, you can render your\nsystem inoperable by entering invalid information.\n\nEnter FLASH address in hex or cancel:\nmover.bin at c0000\narb.bin at d0000", QLineEdit::Normal,"", &ok );
    if(ok)
    {
        QString fileName = QFileDialog::getOpenFileName(this, tr("File to upload to ARB FLASH"),tr("All files (*)", 0));
        if(fileName != "")
        {
            if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal") disconnect(comms, SIGNAL(DataReady()),0,0);
            comms->ARBupload(Faddress, fileName);
            if( ui->tabMIPS->tabText(ui->tabMIPS->currentIndex()) == "Terminal")
            {
                connect(comms, SIGNAL(DataReady()), this, SLOT(readData2Console()));
                readData2Console();
            }
        }
    }
}

void MIPS::CloseControlPanel(void)
{
    cp_deleteRequest = true;
    //this->setWindowState(Qt::WindowMaximized);
    this->setWindowState(Qt::WindowActive);
    ui->tabMIPS->setDisabled(false);
}

void MIPS::SelectCP(QString fileName)
{
    if(sf   != NULL) return;
    if(sl   != NULL) return;
    if(sl2  != NULL) return;
    if(grid != NULL) return;
    if(cp   != NULL) return;
    ui->tabMIPS->setCurrentIndex(0);
    ControlPanel *c = new ControlPanel(0,fileName,Systems,properties);
    if(!c->LoadedConfig) return;
//    c->show();
    connect(c, SIGNAL(DialogClosed()), this, SLOT(CloseControlPanel()));
    c->Update();
    this->setWindowState(Qt::WindowMinimized);
    cp = c;
    cp->show();
    ui->tabMIPS->setDisabled(true);
    cp->raise();
}

void MIPS::slotScripting(void)
{
    if(!scriptconsole) scriptconsole = new ScriptingConsole(this,properties);
    scriptconsole->show();
}

// Returns a pointer to the comm port for the MIPS system defined my its name.
// Returns null if the MIPS system can't be found.
Comms* MIPS::FindCommPort(QString name, QList<Comms*> Systems)
{
   for(int i = 0; i < Systems.length(); i++)
   {
       if(Systems.at(i)->MIPSname == name) return(Systems.at(i));
   }
   return NULL;
}



bool MIPS::SendCommand(QString message)
{
   return comms->SendCommand(message);
}

QString MIPS::SendMess(QString message)
{
   return comms->SendMess(message);
}

bool MIPS::SendCommand(QString MIPSname, QString message)
{
    QApplication::processEvents();
    Comms *cp =  FindCommPort(MIPSname,Systems);
    if(cp==NULL) return false;
    return cp->SendCommand(message);
}

QString MIPS::SendMess(QString MIPSname, QString message)
{
    QApplication::processEvents();
    Comms *cp =  FindCommPort(MIPSname,Systems);
    if(cp==NULL) return "MIPS not found!";
    return cp->SendMess(message);
}

void MIPS::msDelay(int ms)
{
    QTime timer;

    timer.start();
    while(timer.elapsed() < ms) QApplication::processEvents();
}

void MIPS::statusMessage(QString message)
{
    QApplication::processEvents();
    ui->statusBar->showMessage(message);
    QApplication::processEvents();
}

void MIPS::popupMessage(QString message)
{
    QMessageBox msgBox;

    QApplication::processEvents();
    msgBox.setText(message);
    msgBox.setInformativeText("");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

bool MIPS::popupYesNoMessage(QString message)
{
    QMessageBox msgBox;

    QApplication::processEvents();
    msgBox.setText(message);
    msgBox.setInformativeText("Are you sure?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();
    if(ret == QMessageBox::No) return false;
    return true;
}

QString MIPS::popupUserInput(QString title, QString message)
{
    bool ok;

    QApplication::processEvents();
    QString text = QInputDialog::getText(this, title, message, QLineEdit::Normal,QString::null, &ok);
    if(!ok) text="";
    QApplication::processEvents();
    return text;
}

void MIPS::slotLogStatusBarMessage(QString statusMess)
{
    if(statusMess == "") return;
    if(statusMess.isEmpty()) return;
    if(properties != NULL) properties->Log("MIPS StatusBar: " + statusMess);
}
