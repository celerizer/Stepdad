#include "mainwindow.h"
#include "ui_mainwindow.h"

extern "C"
{
  #include "libh8300h/frontend.h"
}

#include <QtGamepad/QGamepad>
#include <QTimer>

static QGamepad gamepad;

static h8_bool reached = FALSE;

#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QTextStream>
#include <QToolBar>
#include <QLayout>
#include <QAudioDeviceInfo>
#include <QAudioFormat>
#include <QKeyEvent>

using namespace Qt;

void MainWindow::onFrame(void)
{
  unsigned long long instructions = 0;
  char output_buf[1024];
  int i;

  h8_bool button = gamepad.buttonA() || m_KeyDown;
  h8_bool left = gamepad.buttonLeft() || m_KeyLeft;
  h8_bool right = gamepad.buttonRight() || m_KeyRight;
  bool step = stepPulseActive(m_Clock.nsecsElapsed());
  h8_word_t analog;

  if (!m_Loaded)
    return;

  // At rest the NTR-032's BMA150 reads 1 g (256) on Z and the NTR-027's
  // analog sensor reads mid-scale; a step briefly pushes both up
  analog.u = (step ? 704 : 512) << 6;
  for (i = 0; i < m_System.device_count; i++)
  {
    if (m_System.devices[i].type == H8_DEVICE_3BUTTON)
    {
      ((h8_bool*)m_System.devices[i].data)[0] = button;
      ((h8_bool*)m_System.devices[i].data)[1] = left;
      ((h8_bool*)m_System.devices[i].data)[2] = right;
    }
    else if (m_System.devices[i].type == H8_DEVICE_1BUTTON)
      ((h8_bool*)m_System.devices[i].data)[0] = button;
    else if (m_System.devices[i].type == H8_DEVICE_BMA150)
      h8_bma150_set_axis(&m_System.devices[i], 0, 0, step ? 600 : 256);
    else if (m_System.devices[i].type == H8_DEVICE_ACCELEROMETER_X ||
             m_System.devices[i].type == H8_DEVICE_ACCELEROMETER_Y)
      h8_generic_adrr_set(&m_System.devices[i], analog);
  }

  // Run emulation up to the current real time, measured in CPU states so
  // timers and the buzzer run at their real rates. This is called about every
  // millisecond rather than once per frame: an IR reply can only reach the
  // other instance when it next runs, and the NTR-032 gives up waiting for a
  // reply after about 98 ms.
  qint64 now = m_Clock.nsecsElapsed();
  qint64 target = now / 1000 * m_System.clock / 1000000;

  // After a stall (e.g. the window being dragged), skip ahead instead of
  // running a burst of catch-up time
  if (target - m_StatesRun > (qint64)m_System.clock / 10)
    m_StatesRun = target - m_System.clock / 60;

  {
    while (m_StatesRun < target && !m_System.error_code)
    {
      h8_step(&m_System);
      m_StatesRun += m_System.step_states;
      instructions++;

        #if 0
        if (m_System.cpu.pc >= 0xb170 && m_System.cpu.pc < 0xb1ac)
  {
        // Open the dump file
        QFile dumpFile("/home/keith/libh8300h/dump-stepdad.txt");
        QTextStream out(&dumpFile);

        dumpFile.open(QIODevice::WriteOnly | QIODevice::Text);

        do {
            out << QString::number(m_System.instructions).toUpper() << " ";
            out << QString("%1 ").arg(m_System.cpu.pc, 4, 16, QChar('0')).toUpper();
            for (int i = 0; i < 8; ++i) {
                out << QString("er%1:%2 ").arg(i)
                       .arg(m_System.cpu.regs[i].er.u, 8, 16, QChar('0'));
            }
            #if 0
            out << "[" << (m_System.cpu.ccr.flags.c ? "C" : "  ") <<
                          (m_System.cpu.ccr.flags.v ? "V" : "  ") <<
                          (m_System.cpu.ccr.flags.z ? "Z" : "  ") <<
                          (m_System.cpu.ccr.flags.n ? "N" : "  ") <<
                          (m_System.cpu.ccr.flags.i ? "I" : "  ");
            #endif
            for (int i = 0; i < 32; i++)
              out << QString("%1").arg(m_System.vmem.raw[0xff7f - i].u, 2, 16, QChar('0')).toUpper();
            out << "\n";
            h8_step(&m_System);
            instructions++;
        } while (instructions < 70000 && !m_System.error_code);

        dumpFile.close();
        exit(0);
  }
  #endif
    }
  }

  pushAudio();

  // Redraw at 60 Hz; the RTC is emulated, so it keeps time on its own
  if (now - m_LastFrameNs >= 1000000000 / 60)
  {
    m_LastFrameNs = now;
    if (m_StepsLabel)
      m_StepsLabel->setText(QString("Steps: %1 / %2")
        .arg(h8_peek_l(&m_System, 0xFCEC).u)
        .arg(h8_peek_l(&m_System, 0xFCF0).u));
    frameBufferWidget->update();
  }
}

bool MainWindow::stepPulseActive(qint64 now)
{
  // A step is a 250 ms push followed by at least 250 ms of rest, so quick
  // presses still reach the step detectors as separate steps. Both ROMs
  // count this pulse at walking pace; the NTR-027 only starts counting after
  // several steps in a row.
  const qint64 pulseNs = 250000000;

  if (now < m_StepPulseEndNs)
    return true;
  if (m_StepsQueued && now >= m_StepReadyNs)
  {
    m_StepsQueued--;
    m_StepPulseEndNs = now + pulseNs;
    m_StepReadyNs = now + 2 * pulseNs;
    return true;
  }
  return false;
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
  if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)
  {
    QKeyEvent *key = static_cast<QKeyEvent*>(event);
    bool pressed = event->type() == QEvent::KeyPress;

    // Holding a key sends repeated release/press pairs; ignoring them keeps
    // a held key held instead of rapidly tapping it
    if (key->isAutoRepeat())
    {
      switch (key->key())
      {
      case Qt::Key_Left:
      case Qt::Key_Right:
      case Qt::Key_Down:
      case Qt::Key_Space:
        return true;
      default:
        break;
      }
    }

    switch (key->key())
    {
    case Qt::Key_Left:
      m_KeyLeft = pressed;
      return true;
    case Qt::Key_Right:
      m_KeyRight = pressed;
      return true;
    case Qt::Key_Down:
      m_KeyDown = pressed;
      return true;
    case Qt::Key_Space:
      // One step per press; holding the key does not repeat it
      if (pressed && m_StepsQueued < 8)
        m_StepsQueued++;
      return true;
    default:
      break;
    }
  }
  else if (event->type() == QEvent::ApplicationDeactivate)
  {
    // Key releases are lost while another application has focus
    m_KeyLeft = m_KeyRight = m_KeyDown = false;
  }

  return QMainWindow::eventFilter(watched, event);
}

void MainWindow::setupAudio(void)
{
  for (unsigned i = 0; i < m_System.device_count; i++)
    if (m_System.devices[i].type == H8_DEVICE_BUZZER)
      m_Buzzer = &m_System.devices[i];
  if (!m_Buzzer)
    return;

  // Signed 16-bit mono PCM in native byte order, as the buzzer produces it
  QAudioFormat format;
  format.setSampleRate(H8_BUZZER_DEFAULT_RATE);
  format.setChannelCount(1);
  format.setSampleSize(16);
  format.setCodec("audio/pcm");
  format.setSampleType(QAudioFormat::SignedInt);
  format.setByteOrder(QSysInfo::ByteOrder == QSysInfo::LittleEndian ?
                      QAudioFormat::LittleEndian : QAudioFormat::BigEndian);

  QAudioDeviceInfo info = QAudioDeviceInfo::defaultOutputDevice();
  if (!info.isFormatSupported(format))
  {
    // The buzzer can generate any rate, so only the rate may be negotiated
    QAudioFormat nearest = info.nearestFormat(format);

    format.setSampleRate(nearest.sampleRate());
    if (!info.isFormatSupported(format))
    {
      qWarning("No supported audio format; sound is disabled.");
      m_Buzzer = nullptr;
      return;
    }
  }
  h8_buzzer_set_rate(m_Buzzer, format.sampleRate());

  m_AudioOutput = new QAudioOutput(info, format, this);

  // About 100 ms of buffering covers timer jitter between frames
  m_AudioOutput->setBufferSize(format.bytesForDuration(100000));
  m_AudioDevice = m_AudioOutput->start();
  if (!m_AudioDevice)
  {
    qWarning("Failed to start audio output; sound is disabled.");
    m_Buzzer = nullptr;
  }
}

void MainWindow::pushAudio(void)
{
  if (!m_Buzzer || !m_AudioDevice)
    return;

  // Write only what the device has room for. Anything left over stays in the
  // buzzer's ring buffer, which drops its oldest samples to bound latency.
  int room = m_AudioOutput->bytesFree() / (int)sizeof(h8_s16);
  h8_s16 samples[H8_BUZZER_BUFFER_SIZE];
  unsigned count = h8_buzzer_read(m_Buzzer, samples,
                                  std::min<unsigned>(room, H8_BUZZER_BUFFER_SIZE));

  if (count)
    m_AudioDevice->write((const char*)samples, count * sizeof(h8_s16));
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  setWindowIcon(QIcon("://assets/icon.png"));
  setWindowTitle("Stepdad");

  gamepad.setDeviceId(0);

  m_ToolBar = addToolBar("Toolbar");

  QAction *loadAction = new QAction("Load ROM", this);
  QAction *serverAction = new QAction("IR Server", this);
  QAction *clientAction = new QAction( "IR Client", this);
  m_ResetAction = new QAction("Reset", this);
  m_ResetAction->setEnabled(false);

  connect(loadAction, &QAction::triggered, this, [this]() {
    QString path = QFileDialog::getOpenFileName(this, "Load ROM", QString(),
                                                "ROM images (*.bin *.rom);;All files (*)");
    if (!path.isEmpty())
      loadRom(path);
  });

  connect(serverAction, &QAction::triggered, this, [this, serverAction, clientAction]() {
    h8_network_ctx_t network;
    snprintf(network.ip, sizeof(network.ip), "127.0.0.1");
    network.port = 0xaaaa;
    network.server = TRUE;
    if (h8_fe_network_init(&network))
    {
      serverAction->setEnabled(false);
      clientAction->setEnabled(false);
    }
    else
      QMessageBox::warning(this, "Warning", QString("Failed to start server:\n\n%1").arg(network.error_message));
  });

  connect(clientAction, &QAction::triggered, this, [this, serverAction, clientAction]() {
    h8_network_ctx_t network;
    snprintf(network.ip, sizeof(network.ip), "127.0.0.1");
    network.port = 0xaaaa;
    network.server = FALSE;
    if (h8_fe_network_init(&network))
    {
      serverAction->setEnabled(false);
      clientAction->setEnabled(false);
    }
    else
      QMessageBox::warning(this, "Warning", QString("Failed to start client:\n\n%1").arg(network.error_message));
  });

  connect(m_ResetAction, &QAction::triggered, this, [this]() { h8_init(&m_System); });

  m_ToolBar->addAction(loadAction);
  m_ToolBar->addAction(serverAction);
  m_ToolBar->addAction(clientAction);
  m_ToolBar->addAction(m_ResetAction);
  m_ToolBar->setFloatable(false);
  m_ToolBar->setMovable(false);

  frameBufferWidget = new FrameBufferWidget(this);
  setCentralWidget(frameBufferWidget);

  // Resize to fit as ROM-specific toolbar items come and go
  layout()->setSizeConstraint(QLayout::SetFixedSize);

  h8_test();

  m_Clock.start();

  // Watch keys application-wide, so they work whichever widget has focus
  qApp->installEventFilter(this);

  QTimer *timer = new QTimer(this);
  timer->setTimerType(Qt::PreciseTimer);
  connect(timer, &QTimer::timeout, this, &MainWindow::onFrame);
  timer->start(1);  // Emulation runs in ~1 ms slices; see onFrame
}

bool MainWindow::loadRom(const QString &romPath)
{
  QFile romFile(romPath);
  if (!romFile.open(QIODevice::ReadOnly))
  {
    QMessageBox::warning(this, "Warning", "Failed to open ROM file.");
    return false;
  }
  QByteArray romData = romFile.readAll();

  h8_system_id id = h8_system_identify((const h8_u8*)romData.constData(), romData.size());
  if (id == H8_SYSTEM_INVALID)
    id = romData.size() > 0x4000 ? H8_SYSTEM_NTR_032 : H8_SYSTEM_NTR_027;

  // The EEPROM is kept next to the ROM, and starts out blank
  QString eepPath = QFileInfo(romPath).path() + "/" +
                    QFileInfo(romPath).completeBaseName() + ".eep";
  QByteArray eepData(64 * 1024, (char)0xFF);
  QFile eepFile(eepPath);
  if (eepFile.exists())
  {
    if (!eepFile.open(QIODevice::ReadOnly))
    {
      QMessageBox::warning(this, "Warning", "Failed to open EEPROM file.");
      return false;
    }
    eepData = eepFile.readAll();
  }

  if (m_Loaded)
    saveEeprom();
  m_Loaded = false;

  if (m_AudioOutput)
  {
    m_AudioOutput->stop();
    delete m_AudioOutput;
    m_AudioOutput = nullptr;
    m_AudioDevice = nullptr;
  }
  m_Buzzer = nullptr;
  qDeleteAll(m_RomActions);
  m_RomActions.clear();
  m_StepsLabel = nullptr;
  frameBufferWidget->setLcd(nullptr, 96, 64);
  frameBufferWidget->setLed(nullptr);

  // The core expects a zeroed system (e.g. unused device slots and hooks)
  memset(&m_System, 0, sizeof(m_System));

  h8_rtc_set_current(&m_System.vmem.parts.io1.rtc, 0);
  memcpy(m_System.vmem.raw, romData.data(), std::min(romData.size(), (int)sizeof(m_System.vmem.raw)));

  h8_init(&m_System);
  h8_system_init(&m_System, id);

  for (unsigned i = 0; i < m_System.device_count; i++)
  {
    if (m_System.devices[i].type == H8_DEVICE_EEPROM_8K ||
        m_System.devices[i].type == H8_DEVICE_EEPROM_64K)
      memcpy(m_System.devices[i].data, eepData.data(),
             std::min(eepData.size(), (int)m_System.devices[i].size));
    else if (m_System.devices[i].type == H8_DEVICE_LCD)
      frameBufferWidget->setLcd((h8_lcd_t*)m_System.devices[i].device, 96, 64);
    else if (m_System.devices[i].type == H8_DEVICE_LED)
      frameBufferWidget->setLed((h8_led_t*)m_System.devices[i].device);
  }

  // Cheat: the NTR-032 keeps the current watt count as a big-endian word at
  // F78E (verified against ntr032.bin only). The device caps it at 9999.
  if (id == H8_SYSTEM_NTR_032)
  {
    QAction *wattsAction = new QAction("+100 W", this);

    connect(wattsAction, &QAction::triggered, this, [this]() {
      h8_word_t watts = h8_peek_w(&m_System, 0xF78E);

      watts.u = std::min(watts.u + 100, 9999);
      h8_poke_w(&m_System, 0xF78E, watts);
    });
    m_ToolBar->addAction(wattsAction);
    m_RomActions.append(wattsAction);
  }
  // The NTR-027 keeps today's steps and the daily target as big-endian longs
  // at FCEC and FCF0 (verified against wwm.rom only)
  else if (id == H8_SYSTEM_NTR_027)
  {
    m_StepsLabel = new QLabel(this);
    m_StepsLabel->setContentsMargins(8, 0, 8, 0);
    m_RomActions.append(m_ToolBar->addWidget(m_StepsLabel));
  }

  m_EepPath = eepPath;
  m_ResetAction->setEnabled(true);
  setWindowTitle(QString("Stepdad - %1").arg(QFileInfo(romPath).fileName()));

  setupAudio();
  m_Clock.restart();
  m_StatesRun = 0;
  m_LastFrameNs = 0;
  m_Loaded = true;

  return true;
}

void MainWindow::saveEeprom(void)
{
  for (unsigned i = 0; i < m_System.device_count; i++)
  {
    if (m_System.devices[i].type == H8_DEVICE_EEPROM_8K ||
        m_System.devices[i].type == H8_DEVICE_EEPROM_64K)
    {
      QFile eepFile(m_EepPath);

      if (!eepFile.open(QIODevice::WriteOnly) ||
          eepFile.write((const char*)m_System.devices[i].data, m_System.devices[i].size) < 0)
        QMessageBox::warning(nullptr, "Warning", "Failed to save EEPROM file.");
      return;
    }
  }
}

MainWindow::~MainWindow()
{
  if (m_Loaded)
    saveEeprom();

  if (m_AudioOutput)
    m_AudioOutput->stop();

  delete ui;
  delete frameBufferWidget;
}
