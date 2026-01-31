#include "HorizonWidget.h"
#include <QPainter>
#include <QtMath>
#include <QPainterPath>

HorizonWidget::HorizonWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(250, 160);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void HorizonWidget::setAttitude(double rollDeg, double pitchDeg)
{
    m_rollDeg  = rollDeg;
    m_pitchDeg = pitchDeg;
    update();
}

void HorizonWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const int w = width();
    const int h = height();
    const QPointF c(w * 0.5, h * 0.5);

    // Daire mask (Mission Planner gibi)
    const double R = qMin(w, h) * 0.48;
    QPainterPath clip;
    clip.addEllipse(c, R, R);
    p.setClipPath(clip);

    // Pitch clamp (ufuk çizgisi aşırı kaçmasın)
    double pitch = clamp(m_pitchDeg, -45.0, 45.0);
    double roll  = m_rollDeg;

    // Deg->pixel ölçeği: 1 derece kaç pixel kaydıracak?
    // Mission Planner hissi için R/30 iyi başlangıç (30° -> yarıçap kadar)
    const double pxPerDeg = R / 30.0;
    const double pitchOffsetPx = pitch * pxPerDeg;

    // Dünya (sky/ground) için transform:
    // 1) merkeze gel
    // 2) roll kadar döndür
    // 3) pitch kadar yukarı/aşağı kaydır (döndürülmüş eksende)
    p.translate(c);
    p.rotate(-roll);              // eksi: ekranda sağa yatma hisleri için
    p.translate(0, pitchOffsetPx);

    // Arka plan: üst mavi (sky), alt yeşil (ground)
    // (clip zaten var; büyük rect çiziyoruz)
    QRectF big(-w, -h, w * 2.0, h * 2.0);

    // Sky
    p.fillRect(QRectF(big.left(), big.top(), big.width(), big.height()/2.0), QColor("#86AEEB"));
    // Ground
    p.fillRect(QRectF(big.left(), 0, big.width(), big.height()/2.0), QColor("#6FAE3A"));

    // Ufuk çizgisi (white)
    QPen horizonPen(Qt::white);
    horizonPen.setWidthF(3.0);
    p.setPen(horizonPen);
    p.drawLine(QPointF(-w, 0), QPointF(w, 0));

    // Pitch ladder (10° aralık, Mission Planner benzeri)
    // çizgiler ufka paralel (y=const), yazılar sağ/sol
    QPen ladderPen(Qt::white);
    ladderPen.setWidthF(2.0);
    p.setPen(ladderPen);

    QFont f = p.font();
    f.setPointSizeF(qMax(8.0, R * 0.08));
    p.setFont(f);

    auto drawPitchMark = [&](int deg){
        const double y = -deg * pxPerDeg; // pitch + yukarı: ladder aşağı kayar, bu yüzden -deg
        const double len = (deg % 20 == 0) ? R * 0.55 : R * 0.35;
        const double gap = R * 0.08;

        // çizgi
        p.drawLine(QPointF(-len, y), QPointF(-gap, y));
        p.drawLine(QPointF(gap,  y), QPointF(len,  y));

        // yazı (20, 10, ... -10, -20)
        const QString t = QString::number(deg);
        // sol
        p.drawText(QPointF(-len - R*0.18, y + R*0.03), t);
        // sağ
        p.drawText(QPointF(len + R*0.08, y + R*0.03), t);
    };

    // -40..+40 arası çiz
    for (int deg = -40; deg <= 40; deg += 10) {
        if (deg == 0) continue;
        drawPitchMark(deg);
    }

    // Transform’u geri al (uçak sembolü sabit kalacak)
    p.resetTransform();
    p.setClipping(false);

    // Daire çerçevesi
    QPen ringPen(QColor("#D8D8D8"));
    ringPen.setWidthF(3.0);
    p.setPen(ringPen);
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(c, R, R);

    // Ortadaki sabit "aircraft" sembolü (kırmızı kanatlar + orta işaret)
    QPen planePen(QColor("#E02020"));
    planePen.setWidthF(4.0);
    p.setPen(planePen);

    const double wing = R * 0.55;
    const double wingGap = R * 0.12;
    const double y0 = c.y();

    // Sol kanat
    p.drawLine(QPointF(c.x() - wing, y0), QPointF(c.x() - wingGap, y0));
    // Sağ kanat
    p.drawLine(QPointF(c.x() + wingGap, y0), QPointF(c.x() + wing, y0));

    // Orta küçük V işareti (flight director gibi)
    QPainterPath v;
    v.moveTo(c.x(), y0 + R*0.10);
    v.lineTo(c.x() - R*0.10, y0 + R*0.20);
    v.lineTo(c.x(), y0 + R*0.14);
    v.lineTo(c.x() + R*0.10, y0 + R*0.20);
    p.drawPath(v);

    // (İstersen üstte roll scale / tick’ler de ekleriz)
}
