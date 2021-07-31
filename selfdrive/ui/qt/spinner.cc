#include "selfdrive/ui/qt/spinner.h"

#include <cstdio>
#include <iostream>
#include <string>

#include <QApplication>
#include <QGridLayout>
#include <QPainter>
#include <QString>
#include <QTransform>

#include "selfdrive/hardware/hw.h"
#include "selfdrive/ui/qt/qt_window.h"
#include "selfdrive/ui/qt/util.h"

TrackWidget::TrackWidget(QWidget *parent) : QWidget(parent) {
  setFixedSize(spinner_size);
  setFixedSize(screenBG_spinner_size);
  setAutoFillBackground(true);
  setPalette(Qt::black);

  // pre-compute all the track imgs. make this a gif instead?
  QPixmap comma_img = QPixmap("../assets/img_spinner_comma.png").scaled(screenBG_spinner_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  QTransform transform(1, 0, 0, 1, width() / 2, height() / 2);
  QPixmap track_img = QPixmap("../assets/img_spinner_track.png").scaled(spinner_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  for (QPixmap &img : track_imgs) {
    img = comma_img;
    QPainter p(&img);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.setTransform(transform.rotate(360 / spinner_fps));
    p.drawPixmap(-width()/8+10, -height()/4+40, track_img);
  }

  m_anim.setDuration(1000);
  m_anim.setStartValue(0);
  m_anim.setEndValue(int(track_imgs.size() -1));
  m_anim.setLoopCount(-1);
  m_anim.start();
  connect(&m_anim, SIGNAL(valueChanged(QVariant)), SLOT(update()));
}

void TrackWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.drawPixmap(0, 0, track_imgs[m_anim.currentValue().toInt()]);
}

// Spinner

Spinner::Spinner(QWidget *parent) : QWidget(parent) {
  QGridLayout *main_layout = new QGridLayout(this);
  main_layout->setSpacing(0);
  main_layout->setMargin(0);

  main_layout->addWidget(new TrackWidget(this), 0, 0, Qt::AlignHCenter | Qt::AlignVCenter);

  text = new QLabel();
  text->setWordWrap(true);
  text->setVisible(true);
  text->setAlignment(Qt::AlignCenter);
  main_layout->addWidget(text, 1, 0, Qt::AlignHCenter);

  progress_bar = new QProgressBar();
  progress_bar->setRange(1, 100);
  progress_bar->setTextVisible(true);
  progress_bar->setVisible(true);
  progress_bar->setFixedHeight(65);
  main_layout->addWidget(progress_bar, 1, 0, Qt::AlignHCenter);

  setStyleSheet(R"(
    Spinner {
      background-color: black;
    }
    QLabel {
      color: white;
      font-size: 20px;
      background-color: transparent;
    }
    QProgressBar {
      background-color: transparent;
      width: 125px;
      border solid white;
      border-radius: 1px;
    }
    QProgressBar::chunk {
      width: 600px;
      border-radius: 1px;
      background-color: white;
    }
  )");

  notifier = new QSocketNotifier(fileno(stdin), QSocketNotifier::Read);
  QObject::connect(notifier, &QSocketNotifier::activated, this, &Spinner::update);
};

void Spinner::update(int n) {
  std::string line;
  std::getline(std::cin, line);

  if (line.length()) {
    bool number = std::all_of(line.begin(), line.end(), ::isdigit);
    text->setVisible(!number);
    progress_bar->setVisible(number);
    text->setText(QString::fromStdString(line));
    if (number) {
      progress_bar->setValue(std::stoi(line));
    }
  }
}

int main(int argc, char *argv[]) {
  initApp();
  QApplication a(argc, argv);
  Spinner spinner;
  setMainWindow(&spinner);
  return a.exec();
}
