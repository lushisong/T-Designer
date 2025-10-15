#include "mainwindow.h"
#include <QApplication>
#include <ActiveQt/QAxWidget>
#include <ActiveQt/QAxObject>
#include <QtSql>
#include "common.h"
#define LIBFilePathName "C:\\TBD\\data\\LdMainData.db"
QSqlDatabase  T_LibDatabase;
QAxWidget *GlobalBackAxWidget = nullptr;
IMxDrawApplication *pApp = nullptr;
bool database_init()
{
    if(T_LibDatabase.isOpen()) T_LibDatabase.close();
    T_LibDatabase = QSqlDatabase::addDatabase("QSQLITE","LdMainData");
    QFile  File(LIBFilePathName);
    if(!File.exists()){
            QMessageBox::warning(nullptr, "错误", "数据库文件不存在",
                                 QMessageBox::Ok,QMessageBox::NoButton);
            return false;
    }
    else
        T_LibDatabase.setDatabaseName(LIBFilePathName);
    if (!T_LibDatabase.open()){
        QMessageBox::warning(nullptr, "错误", "数据库文件打开错误", QMessageBox::Ok,QMessageBox::NoButton);
        return false;
    }
    return true;
}
int main(int argc, char *argv[])
{
qDebug()<<"0";
     QApplication a(argc, argv);
     OleInitialize(0);
     qDebug()<<"1";
     if(!database_init())  return 0;
     qDebug()<<"2";
     MainWindow w;
     w.showMaximized(); 
     return a.exec();
}
