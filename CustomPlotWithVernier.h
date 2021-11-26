#ifndef CUSTOMPLOTWITHVERNIER_H
#define CUSTOMPLOTWITHVERNIER_H

#include <QWidget>
#include <qcustomplot.h>

class CustomPlotWithVernier : public QWidget
{
    enum class VernierType {
        None = 0, X = 1, Y = 2, Cross = 3
    };
    enum class WatcherType {
        __WT_Single_Begin,
        Value = __WT_Single_Begin,
        __WT_Single_End,

        __WT_Double_Begin,
        Range = __WT_Double_Begin,
        Max,
        Min,
        __WT_Double_End,
    };
    static QMap<int,QString> WatcherName;
    struct Vernier {
        QCPItemStraightLine *pItemX = nullptr;      ///< x轴游标
        QCPItemStraightLine *pItemY = nullptr;      ///< y轴游标
        QCPItemText *pLabel = nullptr;              ///< 文字
        VernierType type = VernierType::X;
        bool isMaster = false;
        bool isLock = false;
        QSet<int> watchers;
    };
    struct Watcher {
        QSharedPointer<Vernier> vernier;
        WatcherType type;
        inline bool operator==(const Watcher &othr){return (vernier==othr.vernier)&&(type==othr.type);}
    };

public:
    CustomPlotWithVernier(QWidget *parent = nullptr);

    // QObject interface
public:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;

    // 处理鼠标事件，用以拖动标线
private slots:
    void plotMousePress(QMouseEvent *event);
    void plotMouseRelease(QMouseEvent *event);
    void plotMouseMove(QMouseEvent *event);

public:
    ///
    /// \brief addSingleVernier
    /// 添加单标线
    /// \param type
    /// \return
    ///
    QSharedPointer<Vernier> addSingleVernier(VernierType type);
    ///
    /// \brief addDoubleVernier
    /// 添加双标线
    /// \param type
    ///
    void addDoubleVernier(VernierType type);
    ///
    /// \brief delVernier
    /// 删除标线（如果存在伙伴，会同时删除伙伴）
    /// \param vernier
    ///
    void delVernier(const QSharedPointer<Vernier> &vernier);

    ///
    /// \brief delAllVernier
    /// 删除所有标线
    ///
    void delAllVernier();

private:
    ///
    /// \brief delSingleVernier
    /// 删除单标线
    /// \param vernier
    ///
    void delSingleVernier(const QSharedPointer<Vernier> &vernier);

    // 移动标线
    ///
    /// \brief moveVernierPosition
    /// 按像素坐标移动标线
    /// \param vernier
    /// \param position
    /// \param type
    ///
    void moveVernierPosition(const QSharedPointer<Vernier> &vernier, const QPoint &position, VernierType type = VernierType::Cross);
    ///
    /// \brief moveVernierCoords
    /// 按坐标系移动标线
    /// \param vernier
    /// \param coords
    /// \param type
    ///
    void moveVernierCoords(const QSharedPointer<Vernier> &vernier, const QPointF &coords, VernierType type = VernierType::Cross);

    ///
    /// \brief lockVernier
    /// 锁定标线
    /// \param vernier
    /// \param lock
    ///
    void lockVernier(const QSharedPointer<Vernier> &vernier, bool lock);

private:
    ///
    /// \brief findBuddy
    /// 找伙伴
    /// \param vernier
    /// \return {主,从}或者{单,null}
    ///
    QPair<QSharedPointer<Vernier>,QSharedPointer<Vernier>> findBuddy(const QSharedPointer<Vernier> &vernier) const;

public:
    ///
    /// \brief addWatcher
    /// 添加监视器
    /// \param vernier
    /// \param watcherType
    ///
    void addWatcher(const QSharedPointer<Vernier> &vernier, WatcherType watcherType);
    ///
    /// \brief delWatcher
    /// 移除某监视器
    /// \param vernier
    /// \param watcherType
    ///
    void delWatcher(const QSharedPointer<Vernier> &vernier, WatcherType watcherType);

private:
    ///
    /// \brief updateWatcherData
    /// 更新监视器的数据
    ///
    void updateWatcherData();

private:
    QCustomPlot *m_pCustomPlot = nullptr;
    QPoint m_curPos;                                                                ///< 当前右键时的坐标
    QList<QSharedPointer<Vernier>> m_lsVernier;                                     ///< 所有游标
    QHash<QSharedPointer<Vernier>,QSharedPointer<Vernier>> m_buddyVernier;          ///< 双游标对应关系
    QSharedPointer<Vernier> m_pCurVernier = nullptr;                                ///< 鼠标按下时选中的item（随鼠标按键释放而释放）
    VernierType m_curSelectType;    ///< 鼠标按下时，如果选中了游标，标识游标的坐标类型

    QCPItemPixmap *m_pWatcher = nullptr;
    QPointF m_curWatcherPos;    ///< (选中watcher)鼠标按下时watcher相对鼠标的位置
    QList<Watcher> m_watcherData;   ///< 监控的项
};

#endif // CUSTOMPLOTWITHVERNIER_H
