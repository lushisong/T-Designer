#include "mainwindow.h"
#include <QApplication>
#include <ActiveQt/QAxWidget>
#include <ActiveQt/QAxObject>
#include <QtSql>
#include "common.h"
#define LIBFilePathName "C:\\TBD\\data\\LdMainData.db"
QSqlDatabase  T_LibDatabase;
QAxWidget *GlobalBackAxWidget;
IMxDrawApplication *pApp;
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
        QMessageBox::warning(nullptr, "错误", "打开数据库失败",
                             QMessageBox::Ok,QMessageBox::NoButton);
        return false;
    }
    return true;
}
int main(int argc, char *argv[])
{
qDebug()<<"0";
     QApplication a(argc, argv);
     OleInitialize(0);
     QAxObject object("{4FF8F5E1-8D85-45CC-B58E-BE1CF4A5C3EC}");

     object.dynamicCall("InitMxDrawOcx(const QString&,const QString&,const QString&,const QString&,const QString&)", "", 
      QString::fromLocal8Bit("中国船舶重工集团公司第704研究所"), 
      QString::fromLocal8Bit("电液系统半实物实验验证平台"), "0510-83078024",
      "010ADE5E0DA2A305784A00001F22E8014E9A9FCF5957272AA51F7EA69E974AA5D173220AB9865714670FA0B2ED850000060A177AE70EC20BB6BE0000242ABDE1218C1C87E1F84B3CFA9D1A5FB7E0B8C0A8702F347CEEE340D84B85CBAB639EADA5C7717953A30000262A75D1DCB40BDD2D638097969BF91CB4EFC96862F3DB91F7D7292C97D270AD6EBC8EC0EFB13063444100001A22E98792BAD32A4231E8E85A70D588C1B7B782224023E9D27CD844ED721BC9F99D00000D120712AC0F10BFC62E976BEF515415B18F0000080AB8CA9D8405892A7C0000");

     object.dynamicCall("IniSet(const QString&)", "EnableUndo=Y,"                //启动撤回功能
                        "DisplayPrecision=500,"        //设置显示精度，默认为0,可以取0 ~1000,1000为最高精度了
                        "EnablePropertyWindow=N,"      //启用属性编辑窗口功能，默认为不启用，使用该功能，同时还需要加载PropertyEditor.mrx,属性编辑在这里实现的。
                        "NOSHOWBOORDER=Y,"              //不显示控件的外边框，默认是显示的
                        "DrawGridLine=Y,"
                        "EnableClipboard=Y,"
                        "EnableSingleSelection=Y,"
                        "EnableDeleteKey=N,"
                        "IsHighQualityTTf=Y,"           //设置提高TrueType字体的显示质量，为效率默认是false.
                        "ObjectModifyEvent=Y,"
                        "DynamicHighlight=Y,"
                        "ISDISABLEEDITTEXT=Y"
                        );

qDebug()<<"1";
     if(!database_init())  return 0;
qDebug()<<"2";
     GlobalBackAxWidget=new QAxWidget(("{74A777F8-7A8F-4E7C-AF47-7074828086E2}"));
     MxDrawApplication *App=new MxDrawApplication();
     pApp=(IMxDrawApplication*)App;
     pApp->dynamicCall("DwgToJpg(QString,QString,int,int)","C:/TBD/SPS/SPS_CDP.dwg","C:/TBD/data/TempImage/SPS_CDP.jpg",150,85);
     MainWindow w;
     w.showMaximized(); 
     return a.exec();
}
