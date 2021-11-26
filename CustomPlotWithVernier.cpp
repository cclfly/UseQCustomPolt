#include "CustomPlotWithVernier.h"
#include <QVBoxLayout>

QMap<int,QString> CustomPlotWithVernier::WatcherName = {
    {int(CustomPlotWithVernier::WatcherType::Value) , QStringLiteral("值")},
    {int(CustomPlotWithVernier::WatcherType::Range) , QStringLiteral("范围")},
    {int(CustomPlotWithVernier::WatcherType::Max  ) , QStringLiteral("最大值")},
    {int(CustomPlotWithVernier::WatcherType::Min  ) , QStringLiteral("最小值")},
};

CustomPlotWithVernier::CustomPlotWithVernier(QWidget *parent)
    : QWidget(parent)
    , m_pCustomPlot(new QCustomPlot(this))
    , m_pWatcher(new QCPItemPixmap(m_pCustomPlot))
{
    m_pCustomPlot->setInteractions(/*QCP::iRangeDrag | QCP::iRangeZoom | */QCP::iSelectPlottables);
    m_pCustomPlot->setMouseTracking(true);
    m_pCustomPlot->xAxis2->setVisible(true);
    m_pCustomPlot->xAxis2->setTickLabels(false);
    m_pCustomPlot->xAxis2->setTicks(false);
    m_pCustomPlot->yAxis2->setVisible(true);
    m_pCustomPlot->yAxis2->setTickLabels(false);
    m_pCustomPlot->yAxis2->setTicks(false);

    m_pCustomPlot->installEventFilter(this);

    (new QVBoxLayout(this))->addWidget(m_pCustomPlot);

    connect(m_pCustomPlot, &QCustomPlot::mousePress  , this, &CustomPlotWithVernier::plotMousePress  );
    connect(m_pCustomPlot, &QCustomPlot::mouseRelease, this, &CustomPlotWithVernier::plotMouseRelease);
    connect(m_pCustomPlot, &QCustomPlot::mouseMove   , this, &CustomPlotWithVernier::plotMouseMove   );

    m_pWatcher->setVisible(false);
    m_pWatcher->setLayer("overlay");

    //TEST
    m_pCustomPlot->addGraph();
    m_pCustomPlot->addGraph();
    QVector<double> x(1000);
    QVector<double> y1(1000);
    QVector<double> y2(1000);
    for(int i = 0; i < 1000; ++i)
    {
        x[i] = i/100.;
        y1[i] = sin(x[i])+2;
        y2[i] = cos(x[i])+2;
    }
    m_pCustomPlot->graph(0)->addData(x,y1);
    m_pCustomPlot->graph(1)->addData(x,y2);
    m_pCustomPlot->graph(0)->setPen(QPen(Qt::red));
    m_pCustomPlot->graph(1)->setPen(QPen(Qt::blue));
    auto xRange = std::minmax_element(x.begin(),x.end());
    auto yRange = std::minmax_element(y1.begin(),y1.end());
    m_pCustomPlot->xAxis->setRange(*xRange.first-1, *xRange.second+1);
    m_pCustomPlot->yAxis->setRange(*yRange.first-1, *yRange.second+1);
    m_pCustomPlot->replot();
}

bool CustomPlotWithVernier::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == m_pCustomPlot && event->type() == QEvent::ContextMenu)
    {
        QMenu *menu = new QMenu(this);
        QMenu *sngMenu = menu->addMenu(QStringLiteral("添加标线"));
        connect(sngMenu->addAction("X"), &QAction::triggered, this, [this]{addSingleVernier(VernierType::X);});
        connect(sngMenu->addAction("Y"), &QAction::triggered, this, [this]{addSingleVernier(VernierType::Y);});
        connect(sngMenu->addAction("Cross"), &QAction::triggered, this, [this]{addSingleVernier(VernierType::Cross);});
        QMenu *dblMenu = menu->addMenu(QStringLiteral("添加双标线"));
        connect(dblMenu->addAction("X"), &QAction::triggered, this, [this]{addDoubleVernier(VernierType::X);});
        connect(dblMenu->addAction("Y"), &QAction::triggered, this, [this]{addDoubleVernier(VernierType::Y);});
        connect(dblMenu->addAction("Cross"), &QAction::triggered, this, [this]{addDoubleVernier(VernierType::Cross);});

        QSharedPointer<Vernier> curVernier;
        for(const auto &vernier: qAsConst(m_lsVernier))
        {
            if(vernier && !(vernier->pItemX || vernier->pItemY))
                continue;
            double distance = qInf();
            if(int(vernier->type) & int(VernierType::X)) distance = std::min(distance,vernier->pItemX->selectTest(static_cast<QContextMenuEvent*>(event)->pos(), false));
            if(int(vernier->type) & int(VernierType::Y)) distance = std::min(distance,vernier->pItemY->selectTest(static_cast<QContextMenuEvent*>(event)->pos(), false));
            if(distance < 5)
            {
                curVernier = vernier;
            }
        }

        if(curVernier)
        {
            menu->addSeparator();
            // 删除
            connect(menu->addAction(QStringLiteral("删除")), &QAction::triggered, this, [curVernier,this]{ delVernier(curVernier); });
            // 锁定与解锁
            if(!curVernier->isLock)
            {
                connect(menu->addAction(QStringLiteral("锁定")), &QAction::triggered, this, [curVernier,this]{ lockVernier(curVernier,true); });
            }
            else
            {
                connect(menu->addAction(QStringLiteral("解锁")), &QAction::triggered, this, [curVernier,this]{ lockVernier(curVernier,false); });
            }
            // 添加监视器
            if(curVernier->type == VernierType::X)
            {
                menu->addSeparator();
                QMenu *watcherMenu = menu->addMenu(QStringLiteral("添加监视器"));
                auto buddy = findBuddy(curVernier);
                int begin = (!buddy.second)?int(WatcherType::__WT_Single_Begin):int(WatcherType::__WT_Double_Begin);
                int end = (!buddy.second)?int(WatcherType::__WT_Single_End):int(WatcherType::__WT_Double_End);
                for(int i = begin; i < end; ++i)
                {
                    QAction *act = watcherMenu->addAction(WatcherName[i]);
                    act->setCheckable(true);
                    if(buddy.first->watchers.contains(i))
                    {
                        act->setChecked(true);
                    }
                    connect(act, &QAction::triggered, this, [buddy,i,this](bool checked){
                        if(checked)
                            addWatcher(buddy.first,WatcherType(i));
                        else
                            delWatcher(buddy.first,WatcherType(i));
                    });
                }
            }
        }

        if(!m_lsVernier.isEmpty())
        {
            menu->addSeparator();
            connect(menu->addAction(QStringLiteral("清除所有标线")), &QAction::triggered, this, [this]{ delAllVernier(); });
        }

        m_curPos = static_cast<QContextMenuEvent*>(event)->globalPos();
        menu->exec(m_curPos);
        menu->deleteLater();
    }
    return QWidget::eventFilter(watched, event);
}

void CustomPlotWithVernier::plotMousePress(QMouseEvent *event)
{
    m_pCurVernier = nullptr;
    if(event->button() != Qt::MouseButton::LeftButton)
        return;
    m_curSelectType = VernierType::None;
    // 选中watcher
    static auto fn_bSelectWatcher = [this](const QPoint &pos)->bool{
        QList<QPoint> offset = {{0,0},{1,0},{-1,0},{0,1},{0,-1}};
        QMap<double,int> count;
        for(auto &off: offset)
        {
            count[m_pWatcher->selectTest(off+pos, false)]++;
        }
        bool ret = false;
        for(auto it = count.begin(); it != count.end(); ++it)
        {
            if(it.key()<5)
            {
                ret = true;
            }
        }
        if(!ret)
        {
            auto counts = count.values();
            return ((*std::max_element(counts.begin(),counts.end()))>=4);
        }
        return ret;
    };
    if(m_pWatcher->visible() && fn_bSelectWatcher(event->pos()))
    {
        m_curSelectType = VernierType(-1);
        m_curWatcherPos = m_pWatcher->topLeft->coords() - QPointF(m_pCustomPlot->xAxis->pixelToCoord(event->pos().x()), m_pCustomPlot->yAxis->pixelToCoord(event->pos().y()));
        return;
    }
    // 选中vernier
    for(const auto &vernier: qAsConst(m_lsVernier))
    {
        if(vernier && !(vernier->pItemX || vernier->pItemY))
            continue;
        double distanceX = vernier->pItemX?vernier->pItemX->selectTest(event->pos(), false):qInf();
        double distanceY = vernier->pItemY?vernier->pItemY->selectTest(event->pos(), false):qInf();
        if(std::min(distanceX,distanceY) < 5)
        {
            m_pCurVernier = vernier;
            if(distanceX < 5)
            {
                m_curSelectType = VernierType(int(m_curSelectType) | int(VernierType::X));
            }
            if(distanceY < 5)
            {
                m_curSelectType = VernierType(int(m_curSelectType) | int(VernierType::Y));
            }
        }
    }
}

void CustomPlotWithVernier::plotMouseRelease(QMouseEvent *event)
{
    Q_UNUSED(event)
    m_pCurVernier.reset();
    m_curSelectType = VernierType::None;
}

void CustomPlotWithVernier::plotMouseMove(QMouseEvent *event)
{
    // 移动watcher
    if(m_curSelectType == VernierType(-1))
    {
        double x = m_pCustomPlot->xAxis->pixelToCoord(m_pCustomPlot->mapFromGlobal(event->globalPos()).x());
        double y = m_pCustomPlot->yAxis->pixelToCoord(m_pCustomPlot->mapFromGlobal(event->globalPos()).y());
        m_pWatcher->topLeft->setCoords(QPointF(x,y)+m_curWatcherPos);
        m_pCustomPlot->replot();
        return;
    }

    if(!m_pCurVernier || (!m_pCurVernier->pItemX && !m_pCurVernier->pItemY))
        return;
    if(m_pCurVernier->isLock)
        return;

    QSharedPointer<Vernier> slaver;
    QPointF delta;
    if(m_pCurVernier->isMaster)
    {
        auto buddy = findBuddy(m_pCurVernier);
        if(buddy.second)
        {
            slaver = buddy.second;
            double x1 = (buddy.first->pItemX&&buddy.first->pItemX->point1)?buddy.first->pItemX->point1->coords().x():0;
            double y1 = (buddy.first->pItemY&&buddy.first->pItemY->point1)?buddy.first->pItemY->point1->coords().y():0;
            double x2 = (buddy.second->pItemX&&buddy.second->pItemX->point1)?buddy.second->pItemX->point1->coords().x():0;
            double y2 = (buddy.second->pItemY&&buddy.second->pItemY->point1)?buddy.second->pItemY->point1->coords().y():0;
            delta.setX(x2-x1);
            delta.setY(y2-y1);
        }
    }

    double x = m_pCustomPlot->xAxis->pixelToCoord(m_pCustomPlot->mapFromGlobal(event->globalPos()).x());
    double y = m_pCustomPlot->yAxis->pixelToCoord(m_pCustomPlot->mapFromGlobal(event->globalPos()).y());
    moveVernierCoords(m_pCurVernier, QPointF(x,y), m_curSelectType);
    if(slaver)
    {
        moveVernierCoords(slaver, QPointF(x,y)+delta, m_curSelectType);
    }
    updateWatcherData();

    m_pCustomPlot->replot();
}

QSharedPointer<CustomPlotWithVernier::Vernier> CustomPlotWithVernier::addSingleVernier(VernierType type)
{
    QCPItemText *label = new QCPItemText(m_pCustomPlot);
    label->setLayer("overlay");
    label->setPen(QPen(Qt::black));
    label->setBrush(QColor::fromRgba(0x55dddddd));
    label->setPadding(QMargins(2,2,2,2));
    label->setText("");

    QSharedPointer<Vernier> vernier = QSharedPointer<Vernier>::create();
    vernier->type = type;
    vernier->pLabel = label;

    double x = m_pCustomPlot->xAxis->pixelToCoord(m_pCustomPlot->mapFromGlobal(m_curPos).x());
    double y = m_pCustomPlot->yAxis->pixelToCoord(m_pCustomPlot->mapFromGlobal(m_curPos).y());
    double posX = m_pCustomPlot->xAxis->range().lower;
    double posY = m_pCustomPlot->yAxis->range().lower;
    Qt::Alignment labAlignX = Qt::AlignLeft;
    Qt::Alignment labAlignY = Qt::AlignBottom;

    if(int(vernier->type) & int(VernierType::X))
    {
        QCPItemStraightLine *line = new QCPItemStraightLine(m_pCustomPlot);
        line->setVisible(true);
        line->point1->setCoords({x,0});
        line->point2->setCoords({x,1});
        label->setText(label->text() + QString::number(y));
        vernier->pItemX = line;
        posX = x;
        if(x - m_pCustomPlot->xAxis->range().lower > (m_pCustomPlot->xAxis->range().upper - m_pCustomPlot->xAxis->range().lower)/2)
        {
            labAlignX = Qt::AlignRight;
        }
        else
        {
            labAlignX = Qt::AlignLeft;
        }
    }
    if(int(vernier->type) & int(VernierType::Y))
    {
        QCPItemStraightLine *line = new QCPItemStraightLine(m_pCustomPlot);
        line->setVisible(true);
        line->point1->setCoords({0,y});
        line->point2->setCoords({1,y});
        if(!label->text().isEmpty()) label->setText(label->text() + " , ");
        label->setText(label->text() + QString::number(y));
        vernier->pItemY = line;
        posY = y;
        if(y - m_pCustomPlot->yAxis->range().lower > (m_pCustomPlot->yAxis->range().upper - m_pCustomPlot->yAxis->range().lower)/2)
        {
            labAlignY = Qt::AlignTop;
        }
        else
        {
            labAlignY = Qt::AlignBottom;
        }
    }
    label->position->setCoords(posX,posY);
    label->setPositionAlignment(labAlignX|labAlignY);

    m_lsVernier.append(vernier);
    m_pCustomPlot->replot();

    return vernier;
}

void CustomPlotWithVernier::addDoubleVernier(VernierType type)
{
    QSharedPointer<CustomPlotWithVernier::Vernier> master = addSingleVernier(type);
    master->isMaster = true;
    m_curPos += QPoint(50,50);
    QSharedPointer<CustomPlotWithVernier::Vernier> slaver = addSingleVernier(type);
    m_buddyVernier.insert(master,slaver);
}

void CustomPlotWithVernier::delVernier(const QSharedPointer<Vernier> &vernier)
{
    auto buddy = findBuddy(vernier);
    QList<Watcher> willRemoveWatchers;
    for(auto &watcher: m_watcherData)
    {
        if(watcher.vernier == vernier)
        {
            willRemoveWatchers.append(watcher);
        }
    }
    for(auto &watcher: willRemoveWatchers)
    {
        m_watcherData.removeOne(watcher);
    }
    updateWatcherData();
    if(buddy.second)
    {
        m_buddyVernier.remove(buddy.first);
        delSingleVernier(buddy.first);
        delSingleVernier(buddy.second);
    }
    else
    {
        delSingleVernier(buddy.first);
    }
    m_pCustomPlot->replot();
}

void CustomPlotWithVernier::delAllVernier()
{
    m_watcherData.clear();
    updateWatcherData();
    for(auto &vernier: m_lsVernier)
    {
        if(vernier->pItemX)m_pCustomPlot->removeItem(vernier->pItemX);
        if(vernier->pItemY)m_pCustomPlot->removeItem(vernier->pItemY);
        if(vernier->pLabel)m_pCustomPlot->removeItem(vernier->pLabel);
    }
    m_lsVernier.clear();
    m_buddyVernier.clear();
    m_pCurVernier.reset();
    m_pCustomPlot->replot();
}

void CustomPlotWithVernier::delSingleVernier(const QSharedPointer<Vernier> &vernier)
{
    m_lsVernier.removeOne(vernier);
    if(int(vernier->type) & int(VernierType::X)) m_pCustomPlot->removeItem(vernier->pItemX);
    if(int(vernier->type) & int(VernierType::Y)) m_pCustomPlot->removeItem(vernier->pItemY);
    m_pCustomPlot->removeItem(vernier->pLabel);
}

void CustomPlotWithVernier::moveVernierPosition(const QSharedPointer<Vernier> &vernier, const QPoint &position, VernierType type)
{
    moveVernierCoords(vernier, QPointF(m_pCustomPlot->xAxis->pixelToCoord(position.x()),m_pCustomPlot->yAxis->pixelToCoord(position.y())), type);
}

void CustomPlotWithVernier::moveVernierCoords(const QSharedPointer<Vernier> &vernier, const QPointF &coords, VernierType type)
{
    double x = coords.x();
    double y = coords.y();

    if(vernier->pItemX && int(type)&int(VernierType::X))
    {
        QCPItemStraightLine *pLine = vernier->pItemX;
        pLine->point1->setCoords({x,0});
        pLine->point2->setCoords({x,1});
    }
    if(vernier->pItemY && int(type)&int(VernierType::Y))
    {
        QCPItemStraightLine *pLine = vernier->pItemY;
        pLine->point1->setCoords({0,y});
        pLine->point2->setCoords({1,y});
    }

    double labPosX = m_pCustomPlot->xAxis->range().lower;
    double labPosY = m_pCustomPlot->yAxis->range().lower;
    Qt::Alignment labAlignX = Qt::AlignLeft;
    Qt::Alignment labAlignY = Qt::AlignBottom;
    vernier->pLabel->setText("");
    if(int(vernier->type) & int(VernierType::X))
    {
        labPosX = vernier->pItemX->point1?vernier->pItemX->point1->coords().x():labPosX;
        vernier->pLabel->setText(vernier->pLabel->text() + QString::number(labPosX));
        if(labPosX - m_pCustomPlot->xAxis->range().lower > (m_pCustomPlot->xAxis->range().upper - m_pCustomPlot->xAxis->range().lower)/2)
        {
            labAlignX = Qt::AlignRight;
        }
        else
        {
            labAlignX = Qt::AlignLeft;
        }
    }
    if(int(vernier->type) & int(VernierType::Y))
    {
        labPosY = vernier->pItemY->point1?vernier->pItemY->point1->coords().y():labPosY;
        if(!vernier->pLabel->text().isEmpty()) vernier->pLabel->setText(vernier->pLabel->text() + " , ");
        vernier->pLabel->setText(vernier->pLabel->text() + QString::number(labPosY));
        if(labPosY - m_pCustomPlot->yAxis->range().lower > (m_pCustomPlot->yAxis->range().upper - m_pCustomPlot->yAxis->range().lower)/2)
        {
            labAlignY = Qt::AlignTop;
        }
        else
        {
            labAlignY = Qt::AlignBottom;
        }
    }
    vernier->pLabel->position->setCoords(labPosX,labPosY);
    vernier->pLabel->setPositionAlignment(labAlignX|labAlignY);
}

void CustomPlotWithVernier::lockVernier(const QSharedPointer<Vernier> &vernier, bool lock)
{
    auto buddy = findBuddy(vernier);
    if(buddy.first)
        buddy.first->isLock = lock;
    if(buddy.second)
        buddy.second->isLock = lock;
}

QPair<QSharedPointer<CustomPlotWithVernier::Vernier>, QSharedPointer<CustomPlotWithVernier::Vernier> > CustomPlotWithVernier::findBuddy(const QSharedPointer<Vernier> &vernier) const
{
    QPair<QSharedPointer<Vernier>,QSharedPointer<Vernier>> ret;
    if(m_buddyVernier.contains(vernier))
    {
        ret.first = vernier;
        ret.second = m_buddyVernier[vernier];
    }
    else if(m_buddyVernier.values().contains(vernier))
    {
        for(auto it = m_buddyVernier.begin(); it != m_buddyVernier.end(); ++it)
        {
            if(it.value() == vernier)
            {
                ret.first = it.key();
                ret.second = vernier;
                break;
            }
        }
    }
    else
    {
        ret.first = vernier;
    }
    return ret;
}

void CustomPlotWithVernier::addWatcher(const QSharedPointer<Vernier> &vernier, WatcherType watcherType)
{
    if(vernier->watchers.contains(int(watcherType)))
        return;
    vernier->watchers.insert(int(watcherType));

    Watcher watcher{vernier,watcherType};
    if(m_watcherData.contains(watcher))
    {
        return;
    }
    m_watcherData.append(watcher);

    updateWatcherData();
}

void CustomPlotWithVernier::delWatcher(const QSharedPointer<Vernier> &vernier, WatcherType watcherType)
{
    if(!vernier->watchers.contains(int(watcherType)))
        return;
    vernier->watchers.remove(int(watcherType));

    Watcher watcher{vernier,watcherType};
    if(!m_watcherData.contains(watcher))
    {
        return;
    }
    m_watcherData.removeOne(watcher);

    updateWatcherData();
}

void CustomPlotWithVernier::updateWatcherData()
{
    if(m_watcherData.isEmpty())
    {
        m_pWatcher->setVisible(false);
        m_pCustomPlot->replot();
        return;
    }
    if(!m_pWatcher->visible())
    {
        m_pWatcher->topLeft->setCoords(m_pCustomPlot->xAxis->pixelToCoord(25),m_pCustomPlot->yAxis->pixelToCoord(20));
        m_pWatcher->setVisible(true);
    }
    QStringList tableHeader;
    for(const auto &watcherData: qAsConst(m_watcherData))
    {
        switch (watcherData.type) {
        case WatcherType::Value:
        {
            double x = watcherData.vernier->pItemX->point1->coords().x();
            tableHeader.append(QString::number(x));
            break;
        }
        case WatcherType::Max:
            tableHeader.append(QStringLiteral("Max"));
            break;
        case WatcherType::Min:
            tableHeader.append(QStringLiteral("Min"));
            break;
        case WatcherType::Range:
        {
            auto buddy = findBuddy(watcherData.vernier);
            if(!buddy.second)
            {
                continue;
            }
            double x1 = buddy.first->pItemX->point1->coords().x();
            double x2 = buddy.second->pItemX->point1->coords().x();
            tableHeader.append(QStringLiteral("(%1,%2)").arg(x1).arg(x2));
            break;
        }
        default:
            break;
        }
    }
    QList<QStringList> cols;
    for(const auto &watcherData: qAsConst(m_watcherData))
    {
        if(watcherData.vernier->type != VernierType::X)
            continue;

        QStringList col;
        if (int(watcherData.type) >= int(WatcherType::__WT_Single_Begin) && int(watcherData.type) < int(WatcherType::__WT_Single_End))
        {
            double x = watcherData.vernier->pItemX->point1->coords().x();
            for(auto i = 0; i < m_pCustomPlot->graphCount(); ++i)
            {
                auto data = m_pCustomPlot->graph(i)->data();
                bool aaa;
                auto keyRange = data->keyRange(aaa);
                if(keyRange.lower > x || keyRange.upper < x)
                {
                    col.append("-");
                    continue;
                }
                auto it = data->findBegin(x);
                switch (watcherData.type) {
                case WatcherType::Value:
                    col.append(QString::number(it->value));
                    break;
                default:
                    col.append("");
                    break;
                }
            }
        }
        else if (int(watcherData.type) >= int(WatcherType::__WT_Double_Begin) && int(watcherData.type) < int(WatcherType::__WT_Double_End))
        {
            auto buddy = findBuddy(watcherData.vernier);
            if(!buddy.second)
            {
                col.append("");
                continue;
            }
            double x1 = buddy.first->pItemX->point1->coords().x();
            double x2 = buddy.second->pItemX->point1->coords().x();
            if(x1 > x2)
            {
                std::swap(x1,x2);
            }
            for(auto i = 0; i < m_pCustomPlot->graphCount(); ++i)
            {
                auto data = m_pCustomPlot->graph(i)->data();
                bool aaa = true;
                auto keyRange = data->keyRange(aaa);
                if(keyRange.lower > x2 || keyRange.upper < x1)
                {
                    col.append("-");
                    continue;
                }
                auto it1 = data->findBegin(x1);
                auto it2 = data->findBegin(x2);
                auto dataRange = data->valueRange(aaa, QCP::sdBoth, QCPRange(it1->key,it2->key));
                switch(watcherData.type) {
                case WatcherType::Min:
                    col.append(QString::number(dataRange.lower));
                    break;
                case WatcherType::Max:
                    col.append(QString::number(dataRange.upper));
                    break;
                case WatcherType::Range:
                    col.append(QStringLiteral("(%1,%2)").arg(dataRange.lower).arg(dataRange.upper));
                    break;
                default:
                    col.append("");
                    break;
                }
            }
        }
        cols.append(col);
    }

    // 构造html
    QString ths;
    ths.append(QStringLiteral("<th>Curve</th>"));
    for(const QString &th: tableHeader)
    {
        ths += QStringLiteral("<th>%1</th>").arg(th);
    }
    QString trs;
    trs.append(QStringLiteral("<tr>%1</tr>").arg(ths));
    for(int i = 0; i < m_pCustomPlot->graphCount(); ++i)
    {
        QString tr;
        auto pGraph = m_pCustomPlot->graph(i);
        tr.append(QStringLiteral("<td>%1</td>").arg(pGraph->name()));
        for(const auto &col: cols)
        {
            tr += QStringLiteral("<td>%1</td>").arg(col[i]);
        }
        trs.append(QStringLiteral("<tr style=\"color:%2;\">%1</tr>").arg(tr).arg(pGraph->pen().color().name()));
    }
    QString style = QStringLiteral("<style>table{border-collapse:collapse;}table td{text-align:center;}</style>");
    QString table = QStringLiteral("<table border=1 align=center>%1</table>").arg(trs);
    // 预渲染
    int length = 200*cols.size();
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    {
        QTextDocument dom;
        dom.setHtml(style+table);
        QImage img(length,length,QImage::Format_ARGB32);
        img.fill(Qt::blue);
        QPainter painter(&img);
        dom.drawContents(&painter);
        painter.end();
        for(int i = 0; i < length; ++i)
        {
            if(img.pixelColor(i,i) != Qt::blue)
            {
                for(int j = 0; j <= i; ++j)
                {
                    if(img.pixelColor(j,i) != Qt::blue)
                    {
                        x = j;
                        break;
                    }
                }
                for(int j = 0; j <= i; ++j)
                {
                    if(img.pixelColor(i,j) != Qt::blue)
                    {
                        y = j;
                        break;
                    }
                }
                for(int j = i; j < length; ++j)
                {
                    if(img.pixelColor(i,j) == Qt::blue)
                    {
                        h = (j - y);
                        break;
                    }
                }
                for(int j = i; j < length; ++j)
                {
                    if(img.pixelColor(j,i) == Qt::blue)
                    {
                        w = (j - x);
                        break;
                    }
                }
                break;
            }
        }
    }
    // 渲染
    {
        QTextDocument dom;
        dom.setHtml(style+table);
        QImage img(length,length,QImage::Format_ARGB32);
        img.fill(Qt::transparent);
        QPainter painter(&img);
        dom.drawContents(&painter);
        painter.end();
        img = img.copy(x,y,w,h);
        m_pWatcher->setPixmap(QPixmap::fromImage(img));
    }

    m_pCustomPlot->replot();
}

