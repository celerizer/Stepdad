#ifndef STEPDAD_FRAMEBUFFER_H
#define STEPDAD_FRAMEBUFFER_H

#include <QWidget>
#include <QImage>
#include <QColor>
#include <QPainter>

#include "libh8300h/devices/lcd.h"
#include "libh8300h/devices/led.h"

class FrameBufferWidget : public QWidget
{
  Q_OBJECT

public:
  explicit FrameBufferWidget(QWidget *parent = nullptr);
  void setLcd(const h8_lcd_t *lcd, int width, int height);
  void setLed(const h8_led_t *led);

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  const h8_lcd_t *m_Lcd = nullptr;
  const h8_led_t *m_Led = nullptr;
  int fbWidth;
  int fbHeight;

  bool isDirty(void);
  void drawFrameBuffer(QPainter &painter);

  QColor m_Colors[4];
  QPixmap m_Pixmap;
};

#endif
