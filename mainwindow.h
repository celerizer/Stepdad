#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QAudioSink>
typedef QAudioSink AudioOutput;
#else
#include <QAudioOutput>
typedef QAudioOutput AudioOutput;
#endif
#include <QElapsedTimer>
#include <QLabel>
#include <QMainWindow>
#include <QToolBar>
#include "framebuffer.h"  // Include your FrameBufferWidget header

extern "C"
{
  #include "libh8300h/device.h"
  #include "libh8300h/system.h"
  #include "libh8300h/types.h"
  #include "libh8300h/devices/bma150.h"
  #include "libh8300h/devices/buzzer.h"
  #include "libh8300h/devices/generic_adc.h"
  #include "libh8300h/devices/lcd.h"
};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  void onFrame(void);

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  Ui::MainWindow *ui;
  FrameBufferWidget *frameBufferWidget;
  h8_system_t m_System;

  /** Whether a ROM is loaded and m_System is ready to run */
  bool m_Loaded = false;

  /** Where the loaded ROM's EEPROM contents are saved */
  QString m_EepPath;

  QToolBar *m_ToolBar = nullptr;
  QAction *m_ResetAction = nullptr;

  /** Toolbar items that only apply to the loaded ROM */
  QList<QAction*> m_RomActions;

  bool loadRom(const QString &romPath);
  void saveEeprom(void);

  void setupAudio(void);
  void pushAudio(void);

  /** The system's buzzer device, if it has one */
  h8_device_t *m_Buzzer = nullptr;
  QLabel *m_StepsLabel = nullptr;
  AudioOutput *m_AudioOutput = nullptr;
  QIODevice *m_AudioDevice = nullptr;

  /** Paces emulation to real time, so audio is produced as fast as it plays */
  QElapsedTimer m_Clock;

  /** CPU states emulated since m_Clock started */
  qint64 m_StatesRun = 0;

  /** When the screen was last redrawn, in m_Clock nanoseconds */
  qint64 m_LastFrameNs = 0;

  /** Keyboard state: left/right are the NTR-032's side buttons, down is the
   *  main button on both systems */
  bool m_KeyLeft = false;
  bool m_KeyRight = false;
  bool m_KeyDown = false;

  /** Space presses not yet turned into accelerometer step pulses */
  unsigned m_StepsQueued = 0;

  /** When the current step pulse ends and when the next may start, in
   *  m_Clock nanoseconds */
  qint64 m_StepPulseEndNs = 0;
  qint64 m_StepReadyNs = 0;

  /** Whether an accelerometer step pulse is being applied right now */
  bool stepPulseActive(qint64 now);
};

#endif // MAINWINDOW_H
