#include "framebuffer.h"

// Indexed by h8_led_state: the whole view shows the NTR-027's LED color
static const QColor NTR027_COLORS[H8_LED_STATE_SIZE] =
{
  QColor(0, 0, 0),     /* Invalid */
  QColor(0, 0, 0),     /* Off */
  QColor(255, 0, 0),   /* Red */
  QColor(0, 255, 0),   /* Green */
  QColor(255, 160, 0)  /* Red and green together */
};

static const QColor NTR032_COLORS[4] =
{
  QColor(180, 180, 170),
  QColor(130, 130, 120),
  QColor(100, 100, 90),
  QColor(30, 30, 20)
};

static const QColor MPG002_COLORS[4] =
{
  QColor(0xD7, 0xFE, 0xCF), /* Green */
  QColor(0xFE, 0xE1, 0x01), /* Yellow */
  QColor(0xC4, 0x79, 0x14), /* Red-brown */
  QColor(0x68, 0x0B, 0x7E)  /* Purple-black */
};

static const QColor MPG002SK_COLORS[4] =
{
  QColor(0xE7, 0xF6, 0xEF), /* Off-white */
  QColor(0xF5, 0xD0, 0x5B), /* Peach */
  QColor(0xBC, 0x41, 0x32), /* Red */
  QColor(0x6A, 0x16, 0x61)  /* Purple-black */
};

const int scale = 4;

FrameBufferWidget::FrameBufferWidget(QWidget *parent)
    : QWidget(parent), fbWidth(96), fbHeight(64)
{
  setFixedSize(fbWidth * scale, fbHeight * scale);
  m_Pixmap = QPixmap(size());
  memcpy(m_Colors, NTR032_COLORS, sizeof(NTR032_COLORS));
}

bool FrameBufferWidget::isDirty(void)
{
  if (m_Lcd)
    return (m_Lcd->start_line < 64 && m_Lcd->display_offset < 128);
  else
    return true;
}

void FrameBufferWidget::setLcd(const h8_lcd_t *lcd, int width, int height)
{
  m_Lcd = lcd;
  fbWidth = width;
  fbHeight = height;
  update();
}

void FrameBufferWidget::setLed(const h8_led_t *led)
{
  m_Led = led;
  update();
}

void FrameBufferWidget::paintEvent(QPaintEvent *event)
{
  QWidget::paintEvent(event);

  if (isDirty())
  {
    QPainter painter(&m_Pixmap);
    drawFrameBuffer(painter);
  }

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, false);
  painter.drawPixmap(0, 0, m_Pixmap);
}

void FrameBufferWidget::drawFrameBuffer(QPainter &painter)
{
  if (m_Lcd)
  {
    float contrastFactor = 1.0f - ((m_Lcd->contrast - 22) / 18.0f);

    contrastFactor = qBound(0.0f, contrastFactor, 1.0f);

    for (int i = 1; i < 4; i++)
    {
      m_Colors[i].setRed(qBound(0, int(NTR032_COLORS[i].red() * contrastFactor), 255));
      m_Colors[i].setGreen(qBound(0, int(NTR032_COLORS[i].green() * contrastFactor), 255));
      m_Colors[i].setBlue(qBound(0, int(NTR032_COLORS[i].blue() * contrastFactor), 255));
    }

    // The panel shows 64 of the controller's 128 RAM lines, beginning at the
    // display start line. The NTR-032 double-buffers by drawing into one half
    // and flipping the start line between 0 and 64.
    for (int x = 0; x < 128 - m_Lcd->display_offset; x++)
    {
      for (int y = 0; y < 64; y++)
      {
        int line = (m_Lcd->start_line + y) & 127;
        int pg = (line >> 3) << 8;
        int bit = line & 7;
        uint8_t hi = m_Lcd->vram[pg + x * 2];
        uint8_t lo = m_Lcd->vram[pg + x * 2 + 1];
        int color = ((hi >> bit) & 1) << 1 | ((lo >> bit) & 1);

        painter.fillRect(x * scale, y * scale, scale, scale, m_Colors[color]);
      }
    }
  }
  else if (m_Led)
    painter.fillRect(m_Pixmap.rect(), NTR027_COLORS[m_Led->state]);
}
