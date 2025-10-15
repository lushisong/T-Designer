#include "dialogpagenameset.h"
#include "ui_dialogpagenameset.h"
extern QSqlDatabase  T_ProjectDatabase;
DialogPageNameSet::DialogPageNameSet(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogPageNameSet)
{
    ui->setupUi(this);
    Canceled=true;
    ui->tableWidget->setColumnWidth(1,50);
}

DialogPageNameSet::~DialogPageNameSet()
{
    delete ui;
}
void DialogPageNameSet::HideEdPageName()
{
  ui->EdPageName->setVisible(false);
  ui->labelPageName->setVisible(false);
  ui->BtnOK->pos().setY(ui->EdPageName->pos().y());
  ui->BtnCancel->pos().setY(ui->EdPageName->pos().y());
  //this->setGeometry(this->x(),this->y(),this->width(),this->height()-ui->EdPageName->height());
}

//Mode=1:Page项目代号  Mode=2:Unit项目代号
void DialogPageNameSet::LoadTable(QString PageName,int Mode)
{
   ProMode=Mode;

   ui->tableWidget->setRowCount(3);
   ui->tableWidget->setItem(0,0,new QTableWidgetItem("高层代号"));ui->tableWidget->setItem(0,1,new QTableWidgetItem("="));
   ui->tableWidget->setItem(1,0,new QTableWidgetItem("位置代号"));ui->tableWidget->setItem(1,1,new QTableWidgetItem("+"));
   ui->tableWidget->setItem(2,0,new QTableWidgetItem("文档类型"));ui->tableWidget->setItem(2,1,new QTableWidgetItem("&"));
   if((Mode==1)||(Mode==2)) ui->tableWidget->setRowHidden(2,true);
   else if(Mode==3)
   {
       ui->tableWidget->setRowHidden(0,true);
       ui->tableWidget->setRowHidden(2,true);
   }


   QComboBox *CbGaoCeng=new QComboBox(ui->tableWidget);
   CbGaoCeng->setEditable(true);
   //查询工程数据库
   QSqlQuery query=QSqlQuery(T_ProjectDatabase);
   QString sqlstr=QString("SELECT * FROM ProjectStructure WHERE Structure_ID = '3'");
   query.exec(sqlstr);
   while(query.next())
   {
      bool Existed=false;
      for(int i=0;i<CbGaoCeng->count();i++) if(CbGaoCeng->itemText(i)==query.value("Structure_INT").toString()) { Existed=true;break;}
      if(!Existed) CbGaoCeng->addItem(query.value("Structure_INT").toString());
   }
   if(PageName.contains("="))
   {
       int index=PageName.indexOf("·");

       if(ProMode==1)//Mode=1:Page项目代号  Mode=2:Unit项目代号  Mode=3:Terminal项目代号
       {
           if(PageName.contains("&")) index=PageName.indexOf("&");
           if(PageName.contains("+")) index=PageName.indexOf("+");
           CbGaoCeng->setCurrentText(PageName.mid(PageName.indexOf("=")+1,index-PageName.indexOf("=")-1));
       }
       else
       {
           index=PageName.indexOf("-");
           if(PageName.contains("+")) index=PageName.indexOf("+");
           CbGaoCeng->setCurrentText(PageName.mid(PageName.indexOf("=")+1,index-PageName.indexOf("=")-1));
       }
   }
   else CbGaoCeng->setCurrentText("");
   ui->tableWidget->setCellWidget(0,2,CbGaoCeng);
   sqlstr=QString("SELECT * FROM ProjectStructure WHERE Structure_ID = '3' AND Structure_INT = '"+CbGaoCeng->currentText()+"'");
   query.exec(sqlstr);
   if(query.next()) ui->tableWidget->setItem(0,3,new QTableWidgetItem(query.value("Struct_Desc").toString()));
   else ui->tableWidget->setItem(0,3,new QTableWidgetItem(""));

   QComboBox *CbPos=new QComboBox(ui->tableWidget);
   CbPos->setEditable(true);
   sqlstr=QString("SELECT * FROM ProjectStructure WHERE Structure_ID = '5'");
   query.exec(sqlstr);
   while(query.next())
   {
      bool Existed=false;
      for(int i=0;i<CbPos->count();i++) if(CbPos->itemText(i)==query.value("Structure_INT").toString()) {Existed=true;break;}
      if(!Existed) CbPos->addItem(query.value("Structure_INT").toString());
   }
   if(PageName.contains("+"))
   {
       int index=PageName.indexOf("·");
       if(ProMode==1)//Mode=1:Page项目代号  Mode=2:Unit项目代号  Mode=3:Terminal项目代号
       {
           if(PageName.contains("&")) index=PageName.indexOf("&");
           CbPos->setCurrentText(PageName.mid(PageName.indexOf("+")+1,index-PageName.indexOf("+")-1));
       }
       else
       {
           if(PageName.contains("-"))
           {
               index=PageName.indexOf("-");
               CbPos->setCurrentText(PageName.mid(PageName.indexOf("+")+1,index-PageName.indexOf("+")-1));
           }
           else
           {
               CbPos->setCurrentText(PageName.mid(PageName.indexOf("+")+1,PageName.count()-PageName.indexOf("+")-1));
           }
       }
   }
   else CbPos->setCurrentText("");
   ui->tableWidget->setCellWidget(1,2,CbPos);
   sqlstr=QString("SELECT * FROM ProjectStructure WHERE Structure_ID = '5' AND Structure_INT = '"+CbPos->currentText()+"'");
   query.exec(sqlstr);
   if(query.next()) ui->tableWidget->setItem(1,3,new QTableWidgetItem(query.value("Struct_Desc").toString()));
   else ui->tableWidget->setItem(1,3,new QTableWidgetItem(""));

   if(ProMode!=1) return;

   QComboBox *CbPageType=new QComboBox(ui->tableWidget);
   CbPageType->setEditable(true);
   sqlstr=QString("SELECT * FROM ProjectStructure WHERE Structure_ID = '6'");
   query.exec(sqlstr);
   while(query.next())
   {
      bool Existed=false;
      for(int i=0;i<CbPageType->count();i++) if(CbPageType->itemText(i)==query.value("Structure_INT").toString()) {Existed=true;break;};
      if(!Existed) CbPageType->addItem(query.value("Structure_INT").toString());
   }
   if(PageName.contains("&"))
   {
       int index=PageName.indexOf("·");
       CbPageType->setCurrentText(PageName.mid(PageName.indexOf("&")+1,index-PageName.indexOf("&")-1));
   }
   else CbPageType->setCurrentText("");
   ui->tableWidget->setCellWidget(2,2,CbPageType);
   sqlstr=QString("SELECT * FROM ProjectStructure WHERE Structure_ID = '6' AND Structure_INT = '"+CbPos->currentText()+"'");
   query.exec(sqlstr);
   if(query.next()) ui->tableWidget->setItem(2,3,new QTableWidgetItem(query.value("Struct_Desc").toString()));
   else ui->tableWidget->setItem(2,3,new QTableWidgetItem(""));

   ui->EdPageName->setText(PageName.mid(PageName.indexOf("·")+1,PageName.count()-PageName.indexOf("·")-1));  
}

void DialogPageNameSet::on_BtnOK_clicked()
{
    ReturnPageName="";
    StrGaoceng=((QComboBox *)ui->tableWidget->cellWidget(0,2))->currentText();
    StrPos=((QComboBox *)ui->tableWidget->cellWidget(1,2))->currentText();

    if(StrGaoceng!="") ReturnPageName+="="+((QComboBox *)ui->tableWidget->cellWidget(0,2))->currentText();
    if(StrPos!="") ReturnPageName+="+"+((QComboBox *)ui->tableWidget->cellWidget(1,2))->currentText();
    if(ProMode==1)
    {
        StrPage=((QComboBox *)ui->tableWidget->cellWidget(2,2))->currentText();
        if(StrPage!="") ReturnPageName+="&"+((QComboBox *)ui->tableWidget->cellWidget(2,2))->currentText();
    }
    if(ReturnPageName!="") ReturnPageName+="·";
    ReturnPageName+=ui->EdPageName->text();
    ReturnUnitPro="="+StrGaoceng+"+"+StrPos;
    ReturnTerminalPro="="+StrGaoceng+"+"+StrPos;
    Canceled=false;
    close();
}
void DialogPageNameSet::on_EdPageName_textChanged(const QString &arg1)
{
    if(ProMode!=1) return;
    if(arg1.contains("=")||arg1.contains("+")||arg1.contains("&")||arg1.contains("·"))
    {
        QMessageBox::information(this, "提示信息","页名不能包含下列任何字符\n+=%·", QMessageBox::Yes);
        ui->EdPageName->setText(arg1.mid(0,arg1.count()-1));
        return;
    }
}

void DialogPageNameSet::on_BtnCancel_clicked()
{
    Canceled=true;
    close();
}
