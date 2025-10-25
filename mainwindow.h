#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QPushButton>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    bool eventFilter(QObject *obj, QEvent *event);
    Ui::MainWindow *ui;
    QString expr;
    int target, cursorPos;

    void closeEvent(QCloseEvent *event);

    QMap<int, QPushButton*> keyToButton; // карта: клавиша -> кнопка

    void insertBinaryOp(char op);
    void updateUI();
    void insertUnaryOp(int type);
    int calculateResult();
    void addBracket(char brac);
    void applyUnaryOperation1(const QString &start, const QString &end);
    void showErrorHint(const QString &text, int time);
    void backsp();
    QTimer* backspaceTimer;
    void replace(int a);

private slots:
    void checkResult();
};

#endif // MAINWINDOW_H
