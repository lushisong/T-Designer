﻿#include "dialogunitmanage.h"
#include "ui_dialogunitmanage.h"
#define NodeIconPath "" //"C:/TBD/data/器件库节点图标.png"
//QIcon icontreenode=QIcon(":/Images/本地器件库节点.png");
DialogUnitManage::DialogUnitManage(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogUnitManage)
{
    ui->setupUi(this);
    Qt::WindowFlags windowFlag = Qt::Dialog;
    windowFlag |= Qt::WindowMinimizeButtonHint;
    windowFlag |= Qt::WindowMaximizeButtonHint;
    windowFlag |= Qt::WindowCloseButtonHint;
    setWindowFlags(windowFlag);

    InitTEdit();

    Model = new QStandardItemModel(ui->treeViewUnitGroup);
    ui->treeViewUnitGroup->header()->setVisible(false);
    ui->treeViewUnitGroup->setColumnWidth(0,50);
    ui->treeViewUnitGroup->setModel(Model);

    ui->BtnCountUnit->setVisible(false);//隐藏"器件计数"按钮BtnCountUnit(调试用)

    ui->tableWidgetUnit->verticalHeader()->setVisible(false);
    ui->tableWidgetUnit->setColumnWidth(0,200);//编号
    ui->tableWidgetUnit->setColumnWidth(1,200);//型号
    ui->tableWidgetUnit->setColumnWidth(3,150);//规格
    ui->tableWidgetUnit->setColumnWidth(2,200);//名称
    ui->tableWidgetUnit->setColumnWidth(4,100);//厂家
    ui->tableWidgetUnit->setColumnWidth(5,150);//订货号
    ui->tableWidgetUnit->setColumnWidth(6,50);//描述
    ui->tableWidgetUnit->setColumnWidth(7,220);//备注

    ui->tableWidgetUnitPic->setColumnWidth(0,300);

    ui->tableWidgetSpur->setColumnWidth(0,120);//功能定义
    ui->tableWidgetSpur->setColumnWidth(1,60);//端号
    ui->tableWidgetSpur->setColumnWidth(2,50);//源端口
    ui->tableWidgetSpur->setColumnWidth(3,60);//终端端口
    ui->tableWidgetSpur->setColumnWidth(4,60);//功能子块代号//描述
    ui->tableWidgetSpur->setColumnWidth(5,40);//符号
    ui->tableWidgetSpur->setColumnWidth(6,90);//序号/插头名称
    ui->tableWidgetSpur->setColumnWidth(7,60);//功能类型
    ui->tableWidgetSpur->setColumnWidth(8,60);//测试代价
    ui->tableWidgetSpur->setColumnWidth(9,40);//受控
    ui->tableWidgetSpur->setColumnWidth(10,60);//端子描述
    //ui->tableWidgetSpur->setColumnHidden(8,true);

    ui->tableWidgetStructure->setColumnWidth(0,140);//变量名称
    ui->tableWidgetStructure->setColumnWidth(1,140);//变量类型
    ui->tableWidgetStructure->setColumnWidth(2,100);//初始值
    ui->tableWidgetStructure->setColumnWidth(3,100);//可控制/可观测
    //ui->tableWidgetStructure->setColumnWidth(4,100);//测试代价

    ui->tableRepairInfo->setColumnWidth(0,140);//名称
    ui->tableRepairInfo->setColumnWidth(1,140);//故障模式
    ui->tableRepairInfo->setColumnWidth(2,240);//解决方案
    ui->tableRepairInfo->setColumnWidth(3,140);//所需资源

    ui->tableTerm->setColumnWidth(0,60);
    ui->tableTerm->setColumnWidth(1,40);
    ui->tableTerm->setColumnWidth(2,40);
    ui->tableTerm->setColumnWidth(3,60);
    ui->tableTerm->setColumnWidth(4,50);

    ui->tableWidgetUnit->setFocusPolicy(Qt::NoFocus);
    ui->tableWidgetSpur->setFocusPolicy(Qt::NoFocus);
    ui->tableWidgetStructure->setFocusPolicy(Qt::NoFocus);

    UpdateCbFactory();

    LoadDBGroup();
    ui->BtnAddUnit->setEnabled(false);
    dlgFuncDefine=new DialogFuncDefine(this);
    dlgLoadSymbol=new DialogLoadSymbol(this);

    ui->treeViewUnitGroup->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeViewUnitGroup,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(ShowtreeViewUnitGroupPopMenu(QPoint)));
    ui->BtnPasteUnit->setEnabled(false);

    //ui->tabWidget->removeTab(4);
    ui->tabWidget->setCurrentIndex(0);

    m_scene.setBackgroundBrush(Qt::gray);
    ui->graphicsView->setScene(&m_scene);

    m_dialogTag=new dialogTag(ui->frameTag);
    connect(m_dialogTag,SIGNAL(DrawTag(int,QColor)),this,SLOT(SlotDrawTagWrapper(int,QColor)));
    connect(m_dialogTag,SIGNAL(ChangeColor(QColor)),this,SLOT(SlotChangeColorWrapper(QColor)));

    m_scene_unit.setBackgroundBrush(Qt::gray);
    ui->graphicsView_Unit->setScene(&m_scene_unit);
    m_scene_unit.SetBackGroundImage(QPixmap(""));

    // 获取所有厂家信息
    QSqlQuery queryFactory(T_LibDatabase);
    queryFactory.exec("SELECT Factory_ID, Factory FROM Factory");
    while(queryFactory.next()) {
        factoryMap[queryFactory.value("Factory_ID").toString()] = queryFactory.value("Factory").toString();
    }

    // 获取EquipmentTemplate中的所有Equipment_ID
//    QSqlQuery queryTemplate(T_LibDatabase);
//    queryTemplate.exec("SELECT DISTINCT Equipment_ID FROM EquipmentTemplate");
//    while(queryTemplate.next()) {
//        equipmentID_IN_EquipmentTemplate.insert(queryTemplate.value("Equipment_ID").toString());
//    }
    on_BtnCountUnit_clicked();//器件计数
}

DialogUnitManage::~DialogUnitManage()
{
    delete ui;
    delete dlgFuncDefine;
    delete dlgLoadSymbol;
}

void DialogUnitManage::UpdateCbFactory()
{
    QString OriginalText="";
    if(ui->CbUnitGroup->count()>0)
    {
        OriginalText=ui->CbUnitGroup->currentText();
    }
    ui->CbUnitGroup->clear();
    ui->CbUnitSubGroup->clear();
    ui->CbUnitFactory->clear();
    QSqlQuery QueryFact = QSqlQuery(T_LibDatabase);//设置数据库选择模型
    QString Fact = QString("SELECT Factory FROM Factory");
    QueryFact.exec(Fact);
    while(QueryFact.next()) ui->CbUnitFactory->addItem(QueryFact.value(0).toString());
    if(OriginalText!="") ui->CbUnitFactory->setCurrentText(OriginalText);
}

void DialogUnitManage::ShowtreeViewUnitGroupPopMenu(const QPoint &pos)
{
    if(!ui->treeViewUnitGroup->indexAt(pos).isValid()) return;
    QMenu tree_menu;
    tree_menu.clear();
    //根据点击节点确定菜单内容
    if(ui->treeViewUnitGroup->indexAt(pos).data(Qt::WhatsThisRole).toString()=="Factory")
    {
        QAction actDelete("删除", this);
        tree_menu.addAction(&actDelete);
        connect(&actDelete,SIGNAL(triggered()),this,SLOT(Delete()));
        tree_menu.exec(QCursor::pos());
    }
    else if(ui->treeViewUnitGroup->indexAt(pos).data(Qt::WhatsThisRole).toString()=="0")
    {
        QAction actNewLevel0("新建总类别", this);
        tree_menu.addAction(&actNewLevel0);
        connect(&actNewLevel0,SIGNAL(triggered()),this,SLOT(NewLevel0()));
        QAction actNewLevel("新建子类别", this);
        tree_menu.addAction(&actNewLevel);
        connect(&actNewLevel,SIGNAL(triggered()),this,SLOT(NewLevel()));
        QAction actRename("重命名", this);
        tree_menu.addAction(&actRename);
        connect(&actRename,SIGNAL(triggered()),this,SLOT(Rename()));
        tree_menu.exec(QCursor::pos());
    }
    else if(ui->treeViewUnitGroup->indexAt(pos).data(Qt::WhatsThisRole).toString()=="1")
    {
        QAction actNewLevel("新建子类别", this);
        tree_menu.addAction(&actNewLevel);
        connect(&actNewLevel,SIGNAL(triggered()),this,SLOT(NewLevel()));
        QAction actDelete("删除", this);
        tree_menu.addAction(&actDelete);
        connect(&actDelete,SIGNAL(triggered()),this,SLOT(Delete()));
        QAction actRename("重命名", this);
        tree_menu.addAction(&actRename);
        connect(&actRename,SIGNAL(triggered()),this,SLOT(Rename()));
        tree_menu.exec(QCursor::pos());
    }
    else if(ui->treeViewUnitGroup->indexAt(pos).data(Qt::WhatsThisRole).toString()=="2")
    {
        QAction actDelete("删除", this);
        tree_menu.addAction(&actDelete);
        connect(&actDelete,SIGNAL(triggered()),this,SLOT(Delete()));
        QAction actRename("重命名", this);
        tree_menu.addAction(&actRename);
        connect(&actRename,SIGNAL(triggered()),this,SLOT(Rename()));
        tree_menu.exec(QCursor::pos());
    }
}

void DialogUnitManage::Rename()
{
    if(!ui->treeViewUnitGroup->currentIndex().isValid()) return;
    QDialog *dialogUnitTypeEdit =new QDialog();
    dialogUnitTypeEdit->setWindowTitle("输入名称");
    dialogUnitTypeEdit->setMinimumSize(QSize(300,70));
    QFormLayout *formlayoutNameEdit = new QFormLayout(dialogUnitTypeEdit);

    QVBoxLayout *layout = new QVBoxLayout(nullptr);
    QPushButton *pushbuttonOK = new QPushButton(dialogUnitTypeEdit);
    pushbuttonOK->setText("确认");

    QHBoxLayout *linelayout1= new QHBoxLayout(nullptr);
    QLabel *m_label1 = new QLabel(dialogUnitTypeEdit);
    m_label1->setText("名称");
    QLineEdit *m_LineEdit = new QLineEdit(dialogUnitTypeEdit);
    m_LineEdit->setText(ui->treeViewUnitGroup->currentIndex().data(Qt::DisplayRole).toString());
    linelayout1->addWidget(m_label1);
    linelayout1->addWidget(m_LineEdit);

    layout->addLayout(linelayout1);
    layout->addWidget(pushbuttonOK);
    formlayoutNameEdit->addRow(layout);

    QObject::connect(pushbuttonOK,SIGNAL(clicked()),dialogUnitTypeEdit,SLOT(accept()));
    if (dialogUnitTypeEdit->exec()==QDialog::Accepted)
    {
        QString NewUnitType=m_LineEdit->text();
        if(NewUnitType=="")
        {
            QMessageBox::warning(nullptr, "提示", "名称为空！");
            return;
        }
        QSqlQuery QueryUpdate = QSqlQuery(T_LibDatabase);//设置数据库选择模型
        QString SqlStr="UPDATE Class SET Desc=:Desc WHERE Class_ID = '"+ui->treeViewUnitGroup->currentIndex().data(Qt::UserRole).toString()+"'";
        QueryUpdate.prepare(SqlStr);
        QueryUpdate.bindValue(":Desc",m_LineEdit->text());
        QueryUpdate.exec();
        if(ui->RbFactory->isChecked()) LoadDBFactory();
        else LoadDBGroup();
    }
}

void DialogUnitManage::DeleteUnitByClass_ID(QString Class_ID,QString Factory_ID)
{
    QSqlQuery QueryEquipment = QSqlQuery(T_LibDatabase);//设置数据库选择模型
    QString SqlStr;
    if(Factory_ID=="") SqlStr= "SELECT Equipment_ID FROM Equipment WHERE Class_ID = '"+Class_ID+"'";
    else SqlStr= "SELECT Equipment_ID FROM Equipment WHERE Class_ID = '"+Class_ID+"' AND Factory_ID = '"+Factory_ID+"'";
    QueryEquipment.exec(SqlStr);
    while(QueryEquipment.next())
    {
        QSqlQuery QueryEquipmentTemplate = QSqlQuery(T_LibDatabase);//设置数据库选择模型
        SqlStr = "DELETE FROM EquipmentTemplate WHERE Equipment_ID = '"+QueryEquipment.value(0).toString()+"'";
        QueryEquipmentTemplate.exec(SqlStr);
    }
    SqlStr = "DELETE FROM Equipment WHERE Class_ID = '"+Class_ID+"'";
    QueryEquipment.exec(SqlStr);
}
void DialogUnitManage::Delete()
{
    if(!ui->treeViewUnitGroup->currentIndex().isValid()) return;
    QMessageBox::StandardButton result=QMessageBox::warning(nullptr,QString::fromLocal8Bit("Warning"),"是否确认删除?",
                                                            QMessageBox::Yes|QMessageBox::No,QMessageBox::NoButton);

    if(result!=QMessageBox::Yes)
    {
        return;
    }
    if(ui->treeViewUnitGroup->currentIndex().data(Qt::WhatsThisRole).toString()=="1")//删除器件类别
    {
        QSqlQuery QueryVar = QSqlQuery(T_LibDatabase);//设置数据库选择模型
        QString SqlStr="SELECT Class_ID FROM Class WHERE ParentNo = '"+ui->treeViewUnitGroup->currentIndex().data(Qt::UserRole).toString()+"'";
        QueryVar.exec(SqlStr);
        while(QueryVar.next())
        {
            if(ui->RbType->isChecked()) DeleteUnitByClass_ID(QueryVar.value(0).toString(),"");
            else DeleteUnitByClass_ID(QueryVar.value(0).toString(),ui->treeViewUnitGroup->currentIndex().parent().data(Qt::UserRole).toString());
        }
        if(ui->RbType->isChecked())  DeleteUnitByClass_ID(ui->treeViewUnitGroup->currentIndex().data(Qt::UserRole).toString(),"");
        else DeleteUnitByClass_ID(ui->treeViewUnitGroup->currentIndex().data(Qt::UserRole).toString(),ui->treeViewUnitGroup->currentIndex().parent().data(Qt::UserRole).toString());

        if(ui->RbType->isChecked())
        {
            SqlStr = "DELETE FROM Class WHERE ParentNo = '"+ui->treeViewUnitGroup->currentIndex().data(Qt::UserRole).toString()+"'";
            QueryVar.exec(SqlStr);
            SqlStr = "DELETE FROM Class WHERE Class_ID = '"+ui->treeViewUnitGroup->currentIndex().data(Qt::UserRole).toString()+"'";
            QueryVar.exec(SqlStr);
        }
    }
    else if(ui->treeViewUnitGroup->currentIndex().data(Qt::WhatsThisRole).toString()=="2")//删除器件子类别
    {
        QSqlQuery QueryVar = QSqlQuery(T_LibDatabase);//设置数据库选择模型

        if(ui->RbType->isChecked())
        {
            DeleteUnitByClass_ID(ui->treeViewUnitGroup->currentIndex().data(Qt::UserRole).toString(),"");
            QString SqlStr = "DELETE FROM Class WHERE Class_ID = '"+ui->treeViewUnitGroup->currentIndex().data(Qt::UserRole).toString()+"'";
            QueryVar.exec(SqlStr);
        }
        else
        {
            DeleteUnitByClass_ID(ui->treeViewUnitGroup->currentIndex().data(Qt::UserRole).toString(),ui->treeViewUnitGroup->currentIndex().parent().parent().data(Qt::UserRole).toString());
        }
    }
    if(ui->RbFactory->isChecked()) LoadDBFactory();
    else LoadDBGroup();
}

void DialogUnitManage::NewLevel0()
{
    if(!ui->treeViewUnitGroup->currentIndex().isValid()) return;
    if(ui->treeViewUnitGroup->currentIndex().data(Qt::WhatsThisRole).toString()=="0")
    {
        QDialog *dialogUnitTypeEdit =new QDialog();
        dialogUnitTypeEdit->setWindowTitle("输入总类别");
        dialogUnitTypeEdit->setMinimumSize(QSize(600,100));
        QFormLayout *formlayoutNameEdit = new QFormLayout(dialogUnitTypeEdit);

        QVBoxLayout *layout = new QVBoxLayout(nullptr);
        QPushButton *pushbuttonOK = new QPushButton(dialogUnitTypeEdit);
        pushbuttonOK->setText("确认");

        QHBoxLayout *linelayout1= new QHBoxLayout(nullptr);
        QLabel *m_label1 = new QLabel(dialogUnitTypeEdit);
        m_label1->setText("总类别");
        QLineEdit *m_LineEdit = new QLineEdit(dialogUnitTypeEdit);
        linelayout1->addWidget(m_label1);
        linelayout1->addWidget(m_LineEdit);

        layout->addLayout(linelayout1);
        layout->addWidget(pushbuttonOK);
        formlayoutNameEdit->addRow(layout);

        QObject::connect(pushbuttonOK,SIGNAL(clicked()),dialogUnitTypeEdit,SLOT(accept()));
        if (dialogUnitTypeEdit->exec()==QDialog::Accepted)
        {
            QString NewUnitType=m_LineEdit->text();
            if(NewUnitType=="")
            {
                QMessageBox::warning(this,"提示","总类别为空！");
                return;
            }
            int Class_ID=GetMaxIDOfLibDatabaseByLevel(T_LibDatabase,"Class","Class_ID","0","0");
            //更新T_ProjectDatabase数据库的Equipment表
            QSqlQuery QueryVar = QSqlQuery(T_LibDatabase);//设置数据库选择模型
            QString tempSQL = QString("INSERT INTO Class (Class_ID,ParentNo,Level,Desc,_Order)"
                                      "VALUES (:Class_ID,:ParentNo,:Level,:Desc,:_Order)");
            QueryVar.prepare(tempSQL);
            QueryVar.bindValue(":Class_ID",QString::number(Class_ID));
            QueryVar.bindValue(":ParentNo","0");
            QueryVar.bindValue(":Level","0");
            QueryVar.bindValue(":Desc",NewUnitType);
            QueryVar.bindValue(":_Order",Class_ID);
            QueryVar.exec();
        }
        else return;

        if(ui->RbFactory->isChecked()) LoadDBFactory();
        else LoadDBGroup();
    }
}

void DialogUnitManage::NewLevel()
{
    if(!ui->treeViewUnitGroup->currentIndex().isValid()) return;
    if(ui->treeViewUnitGroup->currentIndex().data(Qt::WhatsThisRole).toString()=="0")//新建器件类别
    {
        QDialog *dialogUnitTypeEdit =new QDialog();
        dialogUnitTypeEdit->setWindowTitle("输入器件类别");
        dialogUnitTypeEdit->setMinimumSize(QSize(600,100));
        QFormLayout *formlayoutNameEdit = new QFormLayout(dialogUnitTypeEdit);

        QVBoxLayout *layout = new QVBoxLayout(nullptr);
        QPushButton *pushbuttonOK = new QPushButton(dialogUnitTypeEdit);
        pushbuttonOK->setText("确认");

        QHBoxLayout *linelayout1= new QHBoxLayout(nullptr);
        QLabel *m_label1 = new QLabel(dialogUnitTypeEdit);
        m_label1->setText("器件类别");
        QLineEdit *m_LineEdit = new QLineEdit(dialogUnitTypeEdit);
        linelayout1->addWidget(m_label1);
        linelayout1->addWidget(m_LineEdit);

        layout->addLayout(linelayout1);
        layout->addWidget(pushbuttonOK);
        formlayoutNameEdit->addRow(layout);

        QObject::connect(pushbuttonOK,SIGNAL(clicked()),dialogUnitTypeEdit,SLOT(accept()));
        if (dialogUnitTypeEdit->exec()==QDialog::Accepted)
        {
            QString NewUnitType=m_LineEdit->text();
            if(NewUnitType=="")
            {
                QMessageBox::warning(this,"提示","器件类别为空！");
                return;
            }
            int Class_ID=GetMaxIDOfLibDatabaseByLevel(T_LibDatabase,"Class","Class_ID","1",ui->treeViewUnitGroup->currentIndex().data(Qt::UserRole).toString());
            //更新T_ProjectDatabase数据库的Equipment表
            QSqlQuery QueryVar = QSqlQuery(T_LibDatabase);//设置数据库选择模型
            QString tempSQL = QString("INSERT INTO Class (Class_ID,ParentNo,Level,Desc,_Order)"
                                      "VALUES (:Class_ID,:ParentNo,:Level,:Desc,:_Order)");
            QueryVar.prepare(tempSQL);
            QueryVar.bindValue(":Class_ID",QString::number(Class_ID));
            QueryVar.bindValue(":ParentNo","1");
            QueryVar.bindValue(":Level","1");
            QueryVar.bindValue(":Desc",NewUnitType);
            QueryVar.bindValue(":_Order",Class_ID%1000);
            QueryVar.exec();
        }
        else return;

        if(ui->RbFactory->isChecked()) LoadDBFactory();
        else LoadDBGroup();
    }
    else if(ui->treeViewUnitGroup->currentIndex().data(Qt::WhatsThisRole).toString()=="1")//新建器件子类别
    {
        QDialog *dialogUnitTypeEdit =new QDialog();
        dialogUnitTypeEdit->setWindowTitle("输入器件子类别");
        dialogUnitTypeEdit->setMinimumSize(QSize(600,100));
        QFormLayout *formlayoutNameEdit = new QFormLayout(dialogUnitTypeEdit);

        QVBoxLayout *layout = new QVBoxLayout(nullptr);
        QPushButton *pushbuttonOK = new QPushButton(dialogUnitTypeEdit);
        pushbuttonOK->setText("确认");

        QHBoxLayout *linelayout1= new QHBoxLayout(nullptr);
        QLabel *m_label1 = new QLabel(dialogUnitTypeEdit);
        m_label1->setText("器件子类别");
        QLineEdit *m_LineEdit = new QLineEdit(dialogUnitTypeEdit);
        linelayout1->addWidget(m_label1);
        linelayout1->addWidget(m_LineEdit);

        layout->addLayout(linelayout1);
        layout->addWidget(pushbuttonOK);
        formlayoutNameEdit->addRow(layout);

        QObject::connect(pushbuttonOK,SIGNAL(clicked()),dialogUnitTypeEdit,SLOT(accept()));
        if (dialogUnitTypeEdit->exec()==QDialog::Accepted)
        {
            QString NewUnitType=m_LineEdit->text();
            int Class_ID=GetMaxIDOfLibDatabaseByLevel(T_LibDatabase,"Class","Class_ID","2",ui->treeViewUnitGroup->currentIndex().data(Qt::UserRole).toString());
            //更新T_ProjectDatabase数据库的Equipment表
            QSqlQuery QueryVar = QSqlQuery(T_LibDatabase);//设置数据库选择模型
            QString tempSQL = QString("INSERT INTO Class (Class_ID,ParentNo,Level,Desc,_Order)"
                                      "VALUES (:Class_ID,:ParentNo,:Level,:Desc,:_Order)");
            QueryVar.prepare(tempSQL);
            QueryVar.bindValue(":Class_ID",QString::number(Class_ID));
            QueryVar.bindValue(":ParentNo",ui->treeViewUnitGroup->currentIndex().data(Qt::UserRole).toString());
            QueryVar.bindValue(":Level","2");
            QueryVar.bindValue(":Desc",NewUnitType);
            QueryVar.bindValue(":_Order",Class_ID%1000);
            QueryVar.exec();
        }
        else return;

        if(ui->RbFactory->isChecked()) LoadDBFactory();
        else LoadDBGroup();
    }
    else if(ui->treeViewUnitGroup->currentIndex().data(Qt::WhatsThisRole).toString()=="2")//新建器件
    {
        on_BtnAddUnit_clicked();
    }
}

void DialogUnitManage::closeEvent(QCloseEvent *event)
{
    if(!ui->tableWidgetUnit->currentIndex().isValid()) return;
    if(RetCode!=1) return;
    emit(SignalCloseWnd(ui->tableWidgetUnit->item(ui->tableWidgetUnit->currentIndex().row(),0)->text(),ui->tableWidgetUnit->item(ui->tableWidgetUnit->currentIndex().row(),2)->text()));
}

//举例： 元件类别分为Level=0：元件  Level=1:高压电器元件  Level=2：接地开关
void DialogUnitManage::LoadDBFactory()
{
    Model->clear();
    ui->CbUnitGroup->clear();
    ui->CbUnitSubGroup->clear();
    ui->CbUnitFactory->clear();
    QSqlQuery QueryFact = QSqlQuery(T_LibDatabase);//设置数据库选择模型
    QString Fact = QString("SELECT Factory FROM Factory");
    QueryFact.exec(Fact);
    while(QueryFact.next()) ui->CbUnitFactory->addItem(QueryFact.value(0).toString());

    QSqlQuery QueryLevel0 = QSqlQuery(T_LibDatabase);//设置数据库选择模型
    QString SqlStr = QString("SELECT * FROM Class WHERE Level = '0'");
    QueryLevel0.exec(SqlStr);
    bool FirstLevel0=true;
    while(QueryLevel0.next())
    {
        QStandardItem *fatherItem;
        fatherItem= new QStandardItem(QIcon(NodeIconPath),QueryLevel0.value("Desc").toString());
        fatherItem->setData(QVariant(QueryLevel0.value("Class_ID").toString()),Qt::UserRole);
        fatherItem->setData(QVariant("0"),Qt::WhatsThisRole);
        Model->appendRow(fatherItem);
        QSqlQuery QueryVar = QSqlQuery(T_LibDatabase);//设置数据库选择模型
        QString temp = QString("SELECT * FROM Factory");
        QueryVar.exec(temp);
        while(QueryVar.next())
        {
            //更新厂家信息，包括图标列表和treeview
            //元件-》ABB-》高压电器元件-》接地开关
            //fatherItem:元件  SubFatherItem:ABB    SubSubItem:高压电器元件   SubSubSubItem:接地开关
            QStandardItem *SubFatherItem=new QStandardItem(QIcon(NodeIconPath),QueryVar.value("Factory").toString());
            SubFatherItem->setData(QVariant(QueryVar.value("Factory_ID").toString()),Qt::UserRole);
            SubFatherItem->setData(QVariant("Factory"),Qt::WhatsThisRole);
            QFileInfo file("C:/TBD/LOGO/"+QueryVar.value(0).toString()+".gif");
            QString gifName;
            if(!file.exists()) gifName="C:/TBD/LOGO/nopic.gif";
            else gifName="C:/TBD/LOGO/"+QueryVar.value(0).toString()+".gif";

            fatherItem->appendRow(SubFatherItem);//厂家添加完成，下面开始添加-》高压电器元件-》接地开关
            QSqlQuery QueryVar2 = QSqlQuery(T_LibDatabase);//设置数据库选择模型
            QString temp2 = QString("SELECT Class_ID FROM Equipment WHERE Factory_ID = '"+QueryVar.value("Factory_ID").toString()+"'");
            QueryVar2.exec(temp2);
            while(QueryVar2.next())
            {
                //SubFatherItem->appendRow(new QStandardItem(QueryVar2.value("Factory").toString()));
                //根据ClassNo确定属于哪个类别
                QString Class_ID=QueryVar2.value(0).toString();
                //qDebug()<<"ClassNo="<<ClassNo;
                QSqlQuery QueryVar3 = QSqlQuery(T_LibDatabase);//设置数据库选择模型
                QString temp3 = QString("SELECT * FROM Class WHERE Class_ID = '"+Class_ID+"'");
                QueryVar3.exec(temp3);
                if(QueryVar3.next())
                {
                    int Level=QueryVar3.value("Level").toInt();
                    QString ParentNo=QueryVar3.value("ParentNo").toString();
                    //qDebug()<<"Level="<<Level<<"ParentNo="<<ParentNo;
                    if(Level==1)//元件为高压电器元件（举例），可为空
                    {
                        //查看当前的Class是否符合Level0
                        if(ParentNo!=QueryLevel0.value("Class_ID").toString()) continue;

                        //查看条目是否存在，不存在则创建
                        bool IsExist=false;
                        for(int i=0;i<SubFatherItem->rowCount();i++)
                        {
                            if(SubFatherItem->child(i)->text()==QueryVar3.value("Desc").toString()) {IsExist=true;break;}
                        }
                        if(!IsExist)
                        {
                            QStandardItem *SubSubItem=new QStandardItem(QIcon(NodeIconPath),QueryVar3.value("Desc").toString());
                            SubSubItem->setData(QVariant(QueryVar3.value("Class_ID").toString()),Qt::UserRole);
                            SubSubItem->setData(QVariant("1"),Qt::WhatsThisRole);
                            SubFatherItem->appendRow(SubSubItem);
                            ui->CbUnitGroup->addItem(QueryVar3.value("Desc").toString());
                        }
                    }
                    else if(Level==2)//接地开关,（举例）
                    {
                        //查看父条目是否存在，不存在则创建
                        //获取父条目的名称
                        QSqlQuery QueryVar4 = QSqlQuery(T_LibDatabase);//设置数据库选择模型
                        QString temp4 = QString("SELECT * FROM Class where Class_ID = '"+ParentNo+"'");
                        QueryVar4.exec(temp4);
                        if(!QueryVar4.next()) continue;

                        //查看当前的Class是否符合Level0
                        QSqlQuery QuerySearch = QSqlQuery(T_LibDatabase);//设置数据库选择模型
                        SqlStr = QString("SELECT * FROM Class WHERE Class_ID = '"+QueryVar4.value("ParentNo").toString()+"'");
                        QuerySearch.exec(temp3);
                        if(QuerySearch.next())
                        {
                            if(QuerySearch.value("ParentNo").toString()!=QueryLevel0.value("Class_ID").toString()) continue;
                        }

                        bool IsExist=false;
                        int SubSubFatherRowIdx=0;
                        for(int i=0;i<SubFatherItem->rowCount();i++)
                        {
                            //qDebug()<<"SubFatherItem->child(i)->text()="<<SubFatherItem->child(i)->text()<<" QueryVar4.value(Desc).toString()="<<QueryVar4.value("Desc").toString();
                            if(SubFatherItem->child(i)->text()==QueryVar4.value("Desc").toString()) {IsExist=true;SubSubFatherRowIdx=i;break;}
                        }
                        if(!IsExist)
                        {
                            QStandardItem *SubSubItem=new QStandardItem(QIcon(NodeIconPath),QueryVar4.value("Desc").toString());
                            SubSubItem->setData(QVariant(QueryVar4.value("Class_ID").toString()),Qt::UserRole);
                            SubSubItem->setData(QVariant("1"),Qt::WhatsThisRole);
                            SubFatherItem->appendRow(SubSubItem);
                            SubSubFatherRowIdx=SubFatherItem->rowCount()-1;
                            ui->CbUnitGroup->addItem(QueryVar4.value("Desc").toString());
                        }

                        //查看条目是否存在，不存在则创建
                        IsExist=false;
                        for(int i=0;i<SubFatherItem->child(SubSubFatherRowIdx)->rowCount();i++)
                        {
                            if(SubFatherItem->child(SubSubFatherRowIdx)->child(i)->text()==QueryVar3.value("Desc").toString()) {IsExist=true;break;}
                        }
                        if(!IsExist)
                        {
                            QStandardItem *SubSubSubItem=new QStandardItem(QIcon(NodeIconPath),QueryVar3.value("Desc").toString());
                            SubSubSubItem->setData(QVariant(QueryVar3.value("Class_ID").toString()),Qt::UserRole);
                            SubSubSubItem->setData(QVariant("2"),Qt::WhatsThisRole);
                            SubFatherItem->child(SubSubFatherRowIdx)->appendRow(SubSubSubItem);
                            ui->CbUnitSubGroup->addItem(QueryVar3.value("Desc").toString());
                        }
                    }
                }
            }
        }
        if(FirstLevel0)
        {
            FirstLevel0=false;
            ui->treeViewUnitGroup->expand(Model->indexFromItem(fatherItem));
            if(Model->rowCount()>0)
            {
                if(Model->item(0,0)->rowCount()>0)
                {
                    ui->treeViewUnitGroup->setCurrentIndex(Model->indexFromItem(Model->item(0,0)->child(0,0)));
                    on_treeViewUnitGroup_clicked(Model->indexFromItem(Model->item(0,0)->child(0,0)));
                }
            }
        }
    }
}

void DialogUnitManage::on_BtnClose_clicked()
{
    Canceled=true;
    RetCode=0;
    close();
}

void DialogUnitManage::LoadDBGroup()
{
    Model->clear();
    ui->CbUnitGroup->clear();
    ui->CbUnitSubGroup->clear();

    QSqlQuery QueryLevel0 = QSqlQuery(T_LibDatabase);//设置数据库选择模型
    QString SqlStr = QString("SELECT * FROM Class WHERE Level = '0'");
    QueryLevel0.exec(SqlStr);
    bool FirstLevel0=true;
    while(QueryLevel0.next())
    {
        QStandardItem *fatherItem;
        fatherItem= new QStandardItem(QIcon(NodeIconPath),QueryLevel0.value("Desc").toString());
        fatherItem->setData(QVariant(QueryLevel0.value("Class_ID").toString()),Qt::UserRole);
        fatherItem->setData(QVariant("0"),Qt::WhatsThisRole);
        Model->appendRow(fatherItem);

        QSqlQuery QueryVar = QSqlQuery(T_LibDatabase);//设置数据库选择模型
        QString temp = QString("SELECT * FROM Class WHERE Level= '1' AND ParentNo = '"+QueryLevel0.value("Class_ID").toString()+"'");
        QueryVar.exec(temp);
        while(QueryVar.next())
        {
            QStandardItem *SubFatherItem=new QStandardItem(QIcon(NodeIconPath),QueryVar.value("Desc").toString());
            SubFatherItem->setData(QVariant(QueryVar.value("Class_ID").toString()),Qt::UserRole);
            //qDebug()<<"Class_ID:"<<QVariant(QueryVar.value("Class_ID").toString());
            SubFatherItem->setData(QVariant("1"),Qt::WhatsThisRole);
            fatherItem->appendRow(SubFatherItem);//下面开始添加-》高压电器元件-》接地开关
            ui->CbUnitGroup->addItem(QueryVar.value("Desc").toString());
            QSqlQuery QueryVar2 = QSqlQuery(T_LibDatabase);//设置数据库选择模型
            QString temp2 = QString("SELECT * FROM Class WHERE Level= '2' AND ParentNo = '"+QueryVar.value("Class_ID").toString()+"'");
            QueryVar2.exec(temp2);
            while(QueryVar2.next())
            {
                QStandardItem *SubSubItem=new QStandardItem(QIcon(NodeIconPath),QueryVar2.value("Desc").toString());
                SubSubItem->setData(QVariant(QueryVar2.value("Class_ID").toString()),Qt::UserRole);
                SubSubItem->setData(QVariant("2"),Qt::WhatsThisRole);
                //qDebug()<<"Class_ID:"<<QueryVar2.value("Class_ID").toString();
                SubFatherItem->appendRow(SubSubItem);//下面开始添加-》高压电器元件-》接地开关
                ui->CbUnitSubGroup->addItem(QueryVar2.value("Desc").toString());
            }
        }
        if(FirstLevel0)
        {
            FirstLevel0=false;
            ui->treeViewUnitGroup->expand(Model->indexFromItem(fatherItem));
            if(Model->rowCount()>0)
            {
                if(Model->item(0,0)->rowCount()>0)
                {
                    ui->treeViewUnitGroup->setCurrentIndex(Model->indexFromItem(Model->item(0,0)->child(0,0)));
                    //on_treeViewUnitGroup_clicked(Model->indexFromItem(Model->item(0,0)->child(0,0)));
                }
            }
        }
    }
}
void DialogUnitManage::on_RbType_clicked(bool checked)
{
    LoadDBGroup();
}
void DialogUnitManage::FindUnitAccordingToDesc(QTableWidget *tablewidget,const QModelIndex &index,int Level)//level=2:接地开关  level=1:高压电器元件
{
    QSqlQuery QueryVar = QSqlQuery(T_LibDatabase);//设置数据库选择模型
    QString temp = QString("SELECT Class_ID FROM Class WHERE Class_ID = '"+index.data(Qt::UserRole).toString()+"'");
    /*
    if(Level==1) temp = QString("SELECT Class_ID FROM Class WHERE Class_ID = '"+index.data(Qt::UserRole).toString()+"'");
    else
    {
        QSqlQuery QueryVar = QSqlQuery(T_LibDatabase);//设置数据库选择模型
        temp=QString("SELECT Class_ID FROM Class WHERE Class_ID = '"+index.parent().data(Qt::UserRole).toString()+"'");
        QueryVar.exec(temp);
        if(!QueryVar.next()) return;
        temp = QString("SELECT Class_ID FROM Class WHERE Desc = '"+index.data().toString()+"' AND ParentNo = '"+QueryVar.value(0).toString()+"'");
    }*/
    QueryVar.exec(temp);
    if(!QueryVar.next()) return;
    QString Class_ID=QueryVar.value(0).toString();
    QueryVar = QSqlQuery(T_LibDatabase);//设置数据库选择模型
    QString FactoryName;
    if(Level==2) FactoryName=index.parent().parent().data().toString();
    else if(Level==1) FactoryName=index.parent().data().toString();
    temp = QString("SELECT Factory_ID FROM Factory WHERE Factory = '"+FactoryName+"'");
    QueryVar.exec(temp);
    if(!QueryVar.next()) return;
    QString Factory_ID=QueryVar.value(0).toString();

    QSqlQuery QueryVar2 = QSqlQuery(T_LibDatabase);//设置数据库选择模型
    QString temp2;
    temp2= QString("SELECT * FROM Equipment WHERE Class_ID = '"+Class_ID+"' AND Factory_ID= '"+Factory_ID+"' ORDER BY PartCode");

    QueryVar2.exec(temp2);
    while(QueryVar2.next())
    {
        tablewidget->setRowCount(tablewidget->rowCount()+1);
        tablewidget->setItem(tablewidget->rowCount()-1,0,new QTableWidgetItem(QueryVar2.value("PartCode").toString()));//编号
        tablewidget->setItem(tablewidget->rowCount()-1,1,new QTableWidgetItem(QueryVar2.value("Type").toString()));//型号
        tablewidget->setItem(tablewidget->rowCount()-1,3,new QTableWidgetItem(QueryVar2.value("Spec").toString()));//规格
        tablewidget->setItem(tablewidget->rowCount()-1,2,new QTableWidgetItem(QueryVar2.value("Name").toString()));//名称
        tablewidget->setItem(tablewidget->rowCount()-1,4,new QTableWidgetItem(FactoryName));//厂家
        tablewidget->setItem(tablewidget->rowCount()-1,5,new QTableWidgetItem(QueryVar2.value("OrderNum").toString()));//订货号
        tablewidget->setItem(tablewidget->rowCount()-1,6,new QTableWidgetItem(QueryVar2.value("Desc").toString()));//描述
        tablewidget->setItem(tablewidget->rowCount()-1,7,new QTableWidgetItem(QueryVar2.value("Remark").toString()));//备注
        tablewidget->item(tablewidget->rowCount()-1,0)->setData(Qt::UserRole,QVariant(QueryVar2.value("Equipment_ID").toString()));
    }
}

void DialogUnitManage::FindUnitAccordingToGroup(QTableWidget *tablewidget,const QModelIndex &index,int Level)//level=2:接地开关  level=1:高压电器元件
{
    // 统计信息
    int greenCount = 0, yellowCount = 0, blueCount = 0, itemCount=0;

    // 获取对应Class_ID的设备
    QSqlQuery queryEquipment(T_LibDatabase);
    queryEquipment.exec(QString("SELECT * FROM Equipment WHERE Class_ID = '%1' ORDER BY PartCode").arg(index.data(Qt::UserRole).toString()));
    while(queryEquipment.next()) {
        int currentRow = tablewidget->rowCount();
        tablewidget->setRowCount(currentRow + 1);

        // 厂家信息
        QString factory = factoryMap.value(queryEquipment.value("Factory_ID").toString(), "未知");
        tablewidget->setItem(currentRow, 4, new QTableWidgetItem(factory));

        // 其他设备信息
        tablewidget->setItem(currentRow, 0, new QTableWidgetItem(queryEquipment.value("PartCode").toString()));
        tablewidget->setItem(currentRow, 1, new QTableWidgetItem(queryEquipment.value("Type").toString()));
        tablewidget->setItem(currentRow, 3, new QTableWidgetItem(queryEquipment.value("Spec").toString()));
        tablewidget->setItem(currentRow, 2, new QTableWidgetItem(queryEquipment.value("Name").toString()));
        tablewidget->setItem(currentRow, 5, new QTableWidgetItem(queryEquipment.value("OrderNum").toString()));
        tablewidget->setItem(currentRow, 6, new QTableWidgetItem(queryEquipment.value("Desc").toString()));
        tablewidget->setItem(currentRow, 7, new QTableWidgetItem(queryEquipment.value("Remark").toString()));
        tablewidget->item(currentRow, 0)->setData(Qt::UserRole, QVariant(queryEquipment.value("Equipment_ID").toString()));

        // 根据记录内容修改行的背景色
//        bool hasPicture = !queryEquipment.value("Picture").toString().isEmpty();
//        bool isInTemplate = equipmentID_IN_EquipmentTemplate.contains(queryEquipment.value("Equipment_ID").toString());
//        if (hasPicture && isInTemplate) {
//            tablewidget->item(currentRow, 0)->setBackground(Qt::green);
//            greenCount++;
//        } else if (hasPicture) {
//            tablewidget->item(currentRow, 0)->setBackground(Qt::yellow);
//            yellowCount++;
//        } else if (isInTemplate) {
//            tablewidget->item(currentRow, 0)->setBackground(Qt::blue);
//            blueCount++;
//        }
//        itemCount++;
    }
//    ui->spinBox_Count->setValue(ui->spinBox_Count->value()+itemCount);
//    ui->spinBox_Green->setValue(ui->spinBox_Green->value()+greenCount);
//    ui->spinBox_Yellow->setValue(ui->spinBox_Yellow->value()+yellowCount);
//    ui->spinBox_Blue->setValue(ui->spinBox_Blue->value()+blueCount);
}

void DialogUnitManage::on_treeViewUnitGroup_clicked(const QModelIndex &index)
{
    if(!index.isValid()) return;
    if(index.data(Qt::WhatsThisRole).toString()=="2")
    {
        if(CopyEquipment_ID!="")  ui->BtnPasteUnit->setEnabled(true);
    }
    else ui->BtnPasteUnit->setEnabled(false);
    ui->BtnAddUnit->setEnabled(false);
    if(ui->RbFactory->isChecked())
    {
        //查找ClassNo，根据ClassNo在EPart中查找对应的元器件
        if(index.parent().isValid())//ABB或 高压电器元件 或  接地开关
        {
            if(index.parent().parent().isValid())//高压电器元件 或  接地开关
            {
                if(index.parent().parent().parent().isValid())//接地开关
                {
                    ui->tableWidgetUnit->setRowCount(0);
                    FindUnitAccordingToDesc(ui->tableWidgetUnit,index,2);
                    ui->BtnAddUnit->setEnabled(true);
                }
                else//高压电器元件
                {
                    //找到高压电器元件下面的类型和高压电器元件本身的ClassNo
                    ui->tableWidgetUnit->setRowCount(0);
                    FindUnitAccordingToDesc(ui->tableWidgetUnit,index,1);
                    for(int i=0;i<Model->itemFromIndex(index)->rowCount();i++)
                    {
                        if(index.child(i,0).data().toString()=="") continue;
                        //qDebug()<<"index.child(i,0).data="<<index.child(i,0).data().toString();
                        FindUnitAccordingToDesc(ui->tableWidgetUnit,index.child(i,0),2);
                    }
                }
            }
            else//ABB
            {
                ui->tableWidgetUnit->setRowCount(0);
                for(int i=0;i<Model->itemFromIndex(index)->rowCount();i++)
                {
                    //qDebug()<<"index.child(i,0).data="<<index.child(i,0).data().toString();
                    //if(index.child(i,0).data().toString()=="") continue;
                    FindUnitAccordingToDesc(ui->tableWidgetUnit,index.child(i,0),1);
                    for(int j=0;j<Model->itemFromIndex(index.child(i,0))->rowCount();j++)
                    {
                        if(index.child(i,0).child(j,0).data().toString()=="") continue;
                        FindUnitAccordingToDesc(ui->tableWidgetUnit,index.child(i,0).child(j,0),2);
                    }
                }
            }
        }
        else//元件
        {
            ui->tableWidgetUnit->setRowCount(0);
            for(int i=0;i<Model->itemFromIndex(index)->rowCount();i++)//厂家
            {
                for(int j=0;j<Model->itemFromIndex(index.child(i,0))->rowCount();j++)//类别
                {
                    FindUnitAccordingToDesc(ui->tableWidgetUnit,index.child(i,0).child(j,0),1);
                    for(int k=0;k<Model->itemFromIndex(index.child(i,0).child(j,0))->rowCount();k++)
                    {
                        //qDebug()<<"index.child(i,0).child(j,0).data="<<index.child(i,0).child(j,0).data().toString();
                        if(index.child(i,0).child(j,0).child(k,0).data().toString()=="") continue;
                        FindUnitAccordingToDesc(ui->tableWidgetUnit,index.child(i,0).child(j,0).child(k,0),2);
                    }
                }
            }
        }
    }//end of if(ui->RbFactory)
    else //Type分类
    {
        if(index.parent().isValid())//高压电器元件 或  接地开关
        {
            if(index.parent().parent().isValid())//接地开关
            {
                ui->tableWidgetUnit->setRowCount(0);
                FindUnitAccordingToGroup(ui->tableWidgetUnit,index,2);
                ui->BtnAddUnit->setEnabled(true);
            }
            else//高压电器元件
            {
                //找到高压电器元件下面的类型和高压电器元件本身的ClassNo
                ui->tableWidgetUnit->setRowCount(0);
                FindUnitAccordingToGroup(ui->tableWidgetUnit,index,1);
                for(int i=0;i<Model->itemFromIndex(index)->rowCount();i++)
                {
                    if(index.child(i,0).data().toString()=="") continue;
                    //qDebug()<<"index.child(i,0).data="<<index.child(i,0).data().toString();
                    FindUnitAccordingToGroup(ui->tableWidgetUnit,index.child(i,0),2);
                }
            }
        }
        else//元件
        {
            ui->tableWidgetUnit->setRowCount(0);
            for(int i=0;i<Model->itemFromIndex(index)->rowCount();i++)//类别
            {
                FindUnitAccordingToGroup(ui->tableWidgetUnit,index.child(i,0),1);
                for(int j=0;j<Model->itemFromIndex(index.child(i,0))->rowCount();j++)//子类别
                {
                    if(index.child(i,0).child(j,0).data().toString()=="") continue;
                    //qDebug()<<"index.child(i,0).data="<<index.child(i,0).data().toString();
                    FindUnitAccordingToGroup(ui->tableWidgetUnit,index.child(i,0).child(j,0),2);
                }
            }
        }
    }
    //默认选中第一个记录
    if(ui->tableWidgetUnit->rowCount()>0)
    {
        ui->tableWidgetUnit->setCurrentIndex(ui->tableWidgetUnit->model()->index(0,0));
        on_tableWidgetUnit_clicked(ui->tableWidgetUnit->currentIndex());
    }
}

//Lu ToDo 从数据库中加载器件信息
void DialogUnitManage::on_tableWidgetUnit_clicked(const QModelIndex &index)
{
    if(index.row()<0) return;

    QString FactoryName=ui->tableWidgetUnit->item(index.row(),4)->text();
    QSqlQuery QueryVar = QSqlQuery(T_LibDatabase);//设置数据库选择模型
    QString temp = QString("SELECT * FROM Factory WHERE Factory = '"+FactoryName+"'");
    QueryVar.exec(temp);
    if(!QueryVar.next()) return;
    QString Factory_ID=QueryVar.value("Factory_ID").toString();
    ui->EdUnitInternet->setText(QueryVar.value("WebSite").toString());

    temp ="SELECT * FROM Equipment WHERE Equipment_ID = '"+ui->tableWidgetUnit->item(index.row(),0)->data(Qt::UserRole).toString()+"'";//Type = '"+ui->tableWidgetUnit->item(index.row(),1)->text()+"' AND Factory_ID='"+Factory_ID+"'");
    QueryVar.exec(temp);
    if(!QueryVar.next()) return;
    CurEquipment_ID=QueryVar.value("Equipment_ID").toString();
    //查看类别和子类别，通过class数据库检索
    QSqlQuery QueryVar2 = QSqlQuery(T_LibDatabase);//设置数据库选择模型
    temp = QString("SELECT * FROM Class WHERE Class_ID = '"+QueryVar.value("Class_ID").toString()+"'");
    QueryVar2.exec(temp);
    if(!QueryVar2.next()) return;
    QString SubType= QueryVar2.value("Desc").toString();
    temp = QString("SELECT * FROM Class WHERE Class_ID = '"+QueryVar2.value("ParentNo").toString()+"'");
    QueryVar2.exec(temp);
    if(!QueryVar2.next()) return;
    QString Type= QueryVar2.value("Desc").toString();

    ui->CbUnitGroup->setCurrentText(Type);
    ui->CbUnitSubGroup->setCurrentText(SubType);
    ui->EdUnitCode->setText(QueryVar.value("PartCode").toString());
    ui->EdUnitType->setText(QueryVar.value("Type").toString());
    ui->EdUnitStandard->setText(QueryVar.value("Spec").toString());
    ui->EdUnitName->setText(QueryVar.value("Name").toString());
    ui->CbUnitFactory->setCurrentText(FactoryName);
    ui->EdOrderNumber->setText(QueryVar.value("OrderNum").toString());
    ui->EdInfo->setText(QueryVar.value("Remark").toString());
    ui->TextEdDesc->setText(QueryVar.value("Desc").toString());
    ui->EdMTBF->setText(QueryVar.value("MTBF").toString());
    //ui->CbModuleType->setCurrentText(QueryVar.value("DataType").toString());
    QsciEdit->setText(QueryVar.value("TModel").toString());
    on_BtnCompile_clicked();
    QStringList ListStructure=QueryVar.value("Structure").toString().split(";");
    if(ListStructure.count()==ui->tableWidgetStructure->rowCount())
    {
        for(int i=0;i<ListStructure.count();i++)
        {
            if(ui->tableWidgetStructure->item(i,0)->text()!=ListStructure.at(i).split(",").at(0)) continue;
            ((QComboBox *)ui->tableWidgetStructure->cellWidget(i,2))->setCurrentText(ListStructure.at(i).split(",").at(1));
            ((QComboBox *)ui->tableWidgetStructure->cellWidget(i,3))->setCurrentText(ListStructure.at(i).split(",").at(2));
            //ui->tableWidgetStructure->setItem(i,4,new QTableWidgetItem(ListStructure.at(i).split(",").at(3)));
        }
    }
    //维修信息
    ui->tableRepairInfo->setRowCount(0);
    for(int i=0;i<ui->tableWidgetStructure->rowCount();i++)
    {
        if(ui->tableWidgetStructure->item(i,1)->text()=="ModeType")
        {
            QComboBox *CbModeTypeBox= ((QComboBox *)ui->tableWidgetStructure->cellWidget(i,2));
            for(int j=0;j<CbModeTypeBox->count();j++)
            {
                if((CbModeTypeBox->itemText(j)=="nominal")||(CbModeTypeBox->itemText(j)=="undefined")||(CbModeTypeBox->itemText(j)=="default")) continue;
                if((CbModeTypeBox->itemText(j)=="on")||(CbModeTypeBox->itemText(j)=="off")||(CbModeTypeBox->itemText(j)=="open")||(CbModeTypeBox->itemText(j)=="close")) continue;
                ui->tableRepairInfo->setRowCount(ui->tableRepairInfo->rowCount()+1);
                ui->tableRepairInfo->setItem(ui->tableRepairInfo->rowCount()-1,0,new QTableWidgetItem(ui->tableWidgetStructure->item(i,0)->text()));
                ui->tableRepairInfo->setItem(ui->tableRepairInfo->rowCount()-1,1,new QTableWidgetItem(CbModeTypeBox->itemText(j)));
                ui->tableRepairInfo->setItem(ui->tableRepairInfo->rowCount()-1,2,new QTableWidgetItem(""));
                ui->tableRepairInfo->setItem(ui->tableRepairInfo->rowCount()-1,3,new QTableWidgetItem(""));
            }
        }
    }
    QStringList ListRepairInfo=QueryVar.value("RepairInfo").toString().split("￤￤");
    if(ListRepairInfo.count()==ui->tableRepairInfo->rowCount())
    {
        for(int i=0;i<ListRepairInfo.count();i++)
        {
            if(ListRepairInfo.at(i).split("￤").count()==4)
            {
                if(ui->tableRepairInfo->item(i,0)->text()!=ListRepairInfo.at(i).split("￤").at(0)) continue;
                if(ui->tableRepairInfo->item(i,1)->text()!=ListRepairInfo.at(i).split("￤").at(1)) continue;
                ui->tableRepairInfo->item(i,2)->setText(ListRepairInfo.at(i).split("￤").at(2));
                ui->tableRepairInfo->item(i,3)->setText(ListRepairInfo.at(i).split("￤").at(3));
            }
        }
    }
    if(ui->tableRepairInfo->rowCount()>0)
    {
        ui->tableRepairInfo->setCurrentIndex(ui->tableRepairInfo->model()->index(0,0));
        on_tableRepairInfo_clicked(ui->tableRepairInfo->model()->index(0,0));
    }

    //==========从数据库中加载图片信息==========
    ui->tableWidgetUnitPic->setRowCount(0);
    QStringList strListPicture = QueryVar.value("Picture").toString().split("||");
    QStringList strListPictureToAdd = QueryVar.value("PictureToAdd").toString().split("||");
    QStringList imagesToFind = strListPicture + strListPictureToAdd;
    imagesToFind.removeAll("");

    //qDebug()<<imagesToFind;
    // 尝试在厂商目录下查找图片
    CurEquipment_Supplier = QueryVar.value("Supplier").toString();
    QDir searchDir(QString(PIC_BASE_PATH) + "/" + CurEquipment_Supplier);
    QStringList foundPaths = findImagesInDir(searchDir, imagesToFind);

    // 如果在厂商目录未找到，尝试在PIC_BASE_PATH下查找
    if (!imagesToFind.isEmpty()) {
        QDir baseDir(PIC_BASE_PATH);
        QStringList baseDirFound = findImagesInDir(baseDir, imagesToFind);
        foundPaths.append(baseDirFound);
    }
    //qDebug()<<foundPaths;
    // 处理找到的图片
    for (const QString &foundPath : foundPaths) {
        QString imageName = QFileInfo(foundPath).fileName();
        bool isToAdd = strListPictureToAdd.contains(imageName);

        int currentRow = ui->tableWidgetUnitPic->rowCount();
        ui->tableWidgetUnitPic->setRowCount(currentRow + 1);
        QTableWidgetItem *item = new QTableWidgetItem(imageName);
        item->setData(Qt::UserRole, foundPath);
        if (isToAdd) item->setBackground(QColor(Qt::blue).lighter());
        ui->tableWidgetUnitPic->setItem(currentRow, 0, item);
    }
    if (ui->tableWidgetUnitPic->rowCount() > 0) {
        ui->tableWidgetUnitPic->setCurrentIndex(ui->tableWidgetUnitPic->model()->index(0, 0));
        on_tableWidgetUnitPic_clicked(ui->tableWidgetUnitPic->model()->index(0, 0));//显示图片
    }else m_scene_unit.SetBackGroundImage(QPixmap(""));
    //==========从数据库中加载图片信息结束

    QString StampDwgName=QueryVar.value("MultiLib").toString();
    ui->LbStampName->setText(StampDwgName);
    QString StampJpgName=StampDwgName;
    StampJpgName.replace("dwg","jpg");
    //如果有多个文件，则显示第一个
    if(StampDwgName.contains("||"))
    {
        StampDwgName=StampDwgName.mid(0,StampDwgName.indexOf("||"));
        StampJpgName=StampJpgName.mid(0,StampJpgName.indexOf("||"));
    }
    QString StampJpgPath="";
    if(StampJpgName!="")
    {
        QFile file1("C:/TBD/UserData/MultiLib/"+FactoryName+"/"+StampDwgName);
        QFile file2("C:/TBD/UserData/MultiLib/"+StampDwgName);
        if(file1.exists()) StampJpgPath="C:/TBD/UserData/MultiLib/"+FactoryName+"/"+StampDwgName;
        else if(file2.exists()) StampJpgPath="C:/TBD/UserData/MultiLib/"+StampDwgName;
        else
        {
            //在所有文件夹中检索
            QFileInfoList ListFileInfo=GetFileList("C:/TBD/UserData/MultiLib/");
            for(int i=0;i<ListFileInfo.count();i++)
            {
                if(ListFileInfo.at(i).fileName()==StampDwgName) StampJpgPath=ListFileInfo.at(i).filePath();
            }
        }
    }
    UnitSymbolsView(StampJpgPath,"C:/TBD/data/UserData/MultiLibJpg/"+StampJpgName,ui->LbStampJpg,true);

    //=====加载功能子块=====
    //功能定义，ui->tableWidgetSpur->item(,0):FunctionDefineClass_ID
    //端号，ui->tableWidgetSpur->item(,1):EquipmentTemplate_ID
    temp = QString("SELECT * FROM EquipmentTemplate WHERE Equipment_ID = '"+QueryVar.value("Equipment_ID").toString()+"'");
    QueryVar2.exec(temp);
    ui->tableWidgetSpur->setRowCount(0);
    ui->tableTerm->setRowCount(0);//清空端子表格
    QStringList ListInterConnectInfo;
    while(QueryVar2.next())
    {
        ui->tableWidgetSpur->setRowCount(ui->tableWidgetSpur->rowCount()+1);
        QSqlQuery QueryVar3 = QSqlQuery(T_LibDatabase);//设置数据库选择模型
        temp = QString("SELECT * FROM FunctionDefineClass WHERE FunctionDefineCode = '"+QueryVar2.value("FunDefine").toString()+"'");
        QueryVar3.exec(temp);
        if(QueryVar3.next())
        {
            ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,0,new QTableWidgetItem(QueryVar3.value("FunctionDefineName").toString()));//功能定义
            ui->tableWidgetSpur->item(ui->tableWidgetSpur->rowCount()-1,0)->setFlags(ui->tableWidgetSpur->item(ui->tableWidgetSpur->rowCount()-1,0)->flags()&(~Qt::ItemIsEditable));
            ui->tableWidgetSpur->item(ui->tableWidgetSpur->rowCount()-1,0)->setData(Qt::UserRole,QVariant(QueryVar3.value("FunctionDefineClass_ID").toString()));

        }
        else
        {
            ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,0,new QTableWidgetItem(QueryVar2.value("FunDefine").toString()));//功能定义
            ui->tableWidgetSpur->item(ui->tableWidgetSpur->rowCount()-1,0)->setFlags(ui->tableWidgetSpur->item(ui->tableWidgetSpur->rowCount()-1,0)->flags()&(~Qt::ItemIsEditable));
        }
        ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,1,new QTableWidgetItem(QueryVar2.value("ConnNum").toString()));//端号
        ui->tableWidgetSpur->item(ui->tableWidgetSpur->rowCount()-1,1)->setData(Qt::UserRole,QVariant(QueryVar2.value("EquipmentTemplate_ID").toInt()));

        //==========加载端口==========
        //增加端口表 先从TermInfo中按EquipmentTemplate_ID直接查询，然后再检查结果是否覆盖每个EquipmentTemplate的ConnNum，如果不能覆盖，则自动根据ConnNum进行解析并自动添加。
        //子块代号， ui->tableTerm->item(,0):EquipmentTemplate_ID
        //端号， ui->tableTerm->item(,1):Term_ID
        QString connNumStr = QueryVar2.value("ConnNum").toString();
        QString connDescStr = QueryVar2.value("ConnDesc").toString();
        QString spurTestCost =QueryVar2.value("TestCost").toString();
        QStringList connNumList = connNumStr.simplified().split("￤");
        QStringList connDescList = connDescStr.trimmed().split('\n');
        QString equipmentTemplateId = QueryVar2.value("EquipmentTemplate_ID").toString();
        QString spurDT = QueryVar2.value("SpurDT").toString();

        if(spurDT=="") spurDT=connNumStr;//如果子图代号为空，则直接采用端号

        // 从TermInfo中按EquipmentTemplate_ID直接查询
        QString termInfoQueryStr = QString("SELECT * FROM TermInfo WHERE EquipmentTemplate_ID = %1").arg(equipmentTemplateId);
        QSqlQuery QueryTermInfo(T_LibDatabase);
        QueryTermInfo.exec(termInfoQueryStr);

        QStringList existingTerms;
        while(QueryTermInfo.next()) {
            QString termNum = QueryTermInfo.value("TermNum").toString().simplified();
            if(termNum!="")existingTerms << termNum;
            qDebug()<<"已存在的Term信息,TermID="+QueryTermInfo.value("Term_ID").toString();
            // 添加已存在的Term信息
            addTotableTerm(spurDT, equipmentTemplateId, termNum, "", spurTestCost, &QueryTermInfo, CurEquipment_Supplier);
        }
        // 检查和添加缺失的Term信息
        for (int i = 0; i < connNumList.size(); ++i) {
            QString connNum = connNumList[i].simplified();
            QString connDesc = "";
            if (connDescList.size() == connNumList.size()) {
                // 如果connDescList与connNumList包含相同的字符串，则connDesc在connDescList中取与connNum序号相同
                connDesc = connDescList[i].simplified();
            } else {
                connDesc = connDescStr.simplified(); // 如果有换行符，也要去掉
            }
            if (!connNum.isEmpty() && !existingTerms.contains(connNum)) {
                // 添加缺失的Term信息
                addTotableTerm(spurDT, equipmentTemplateId, connNum, connDesc, spurTestCost,nullptr, CurEquipment_Supplier);
            }
        }

        QTableWidgetItem *ItemSourceConn=new QTableWidgetItem(QueryVar2.value("SourcePrior").toString());//优先级
        if(QueryVar2.value("SourceConn").toBool()) ItemSourceConn->setCheckState(Qt::Checked);
        else
        {
            if((QueryVar2.value("FuncType").toString()=="")||(QueryVar2.value("FuncType").toString()=="接线端口"))
                ItemSourceConn->setCheckState(Qt::Unchecked);
        }
        //ItemSourceConn->setFlags(ItemSourceConn->flags()&(~Qt::ItemIsEditable));

        QTableWidgetItem *ItemExecConn=new QTableWidgetItem("");
        if(QueryVar2.value("ExecConn").toBool()) ItemExecConn->setCheckState(Qt::Checked);
        else
        {
            if((QueryVar2.value("FuncType").toString()=="")||(QueryVar2.value("FuncType").toString()=="接线端口"))
                ItemExecConn->setCheckState(Qt::Unchecked);
        }
        ItemExecConn->setFlags(ItemExecConn->flags()&(~Qt::ItemIsEditable));
        ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,2,ItemSourceConn);//源端口
        ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,3,ItemExecConn);//终端端口

        ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,4,new QTableWidgetItem(QueryVar2.value("SpurDT").toString()));//功能子块代号
        ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,5,new QTableWidgetItem(QueryVar2.value("Symbol").toString()));//符号
        ui->tableWidgetSpur->item(ui->tableWidgetSpur->rowCount()-1,5)->setFlags(ui->tableWidgetSpur->item(ui->tableWidgetSpur->rowCount()-1,5)->flags()&(~Qt::ItemIsEditable));
        //ui->tableWidgetSpur->item(ui->tableWidgetSpur->rowCount()-1,5)->setData(Qt::UserRole,QVariant(QueryVar2.value("EquipmentTemplate_ID").toString()));
        ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,6,new QTableWidgetItem(QueryVar2.value("Designation").toString()));//序号/插头名称
        if(QueryVar2.value("FuncType").toString()=="")
            ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,7,new QTableWidgetItem("接线端口"));//功能类型
        else
            ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,7,new QTableWidgetItem(QueryVar2.value("FuncType").toString()));//功能类型
        ui->tableWidgetSpur->item(ui->tableWidgetSpur->rowCount()-1,7)->setFlags(ui->tableWidgetSpur->item(ui->tableWidgetSpur->rowCount()-1,7)->flags()&(~Qt::ItemIsEditable));
        ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,8,new QTableWidgetItem(QueryVar2.value("TestCost").toString()));//测试代价
        ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,9,new QTableWidgetItem(""));//受控
        ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,10,new QTableWidgetItem(QueryVar2.value("ConnDesc").toString()));//端口描述
        if(QueryVar2.value("InterConnect").toString()!="")
        {
            ListInterConnectInfo.append(QString::number(ui->tableWidgetSpur->rowCount()-1)+","+QueryVar2.value("InterConnect").toString());//受控端口
        }
    }//end of while(QueryVar2.next())
    //=====加载功能子块结束=====

    qDebug()<<"ListInterConnectInfo="<<ListInterConnectInfo;
    for(int i=0;i<ListInterConnectInfo.count();i++)
    {
        for(int j=0;j<ui->tableWidgetSpur->rowCount();j++)
        {
            qDebug()<<"j="<<j<<",ui->tableWidgetSpur->item(j,1)->data(Qt::UserRole).toString()="<<ui->tableWidgetSpur->item(j,1)->data(Qt::UserRole).toString();
            if(ui->tableWidgetSpur->item(j,1)->data(Qt::UserRole).toString()==ListInterConnectInfo.at(i).split(",").at(1))
            {
                ui->tableWidgetSpur->item(ListInterConnectInfo.at(i).split(",").at(0).toInt(),9)->setText(QString::number(j+1));
                break;
            }
        }
    }
    LoadDiagnoseParameters(QueryVar.value("Equipment_ID").toInt());

    if(ui->tableTerm->rowCount() > 0) {
        //qDebug() << "on_tableTerm_clicked";
        ui->tableTerm->setCurrentIndex(ui->tableTerm->model()->index(0, 0));
        on_tableTerm_clicked(ui->tableTerm->currentIndex());
    }
}

//子块代号， ui->tableTerm->item(,0):EquipmentTemplate_ID
//端号， ui->tableTerm->item(,1):Term_ID
void DialogUnitManage::addTotableTerm(const QString &spurDT, const QString &equipTempId, const QString &ConnNum, const QString &ConnDesc, const QString &cost,const QSqlQuery *queryTermInfo, const QString &supplier)
{
    int currentRow = ui->tableTerm->rowCount();
    ui->tableTerm->setRowCount(currentRow + 1);

    ui->tableTerm->setItem(currentRow, 0, new QTableWidgetItem(spurDT));
    ui->tableTerm->item(currentRow, 0)->setData(Qt::UserRole, equipTempId);
    ui->tableTerm->setItem(currentRow, 1, new QTableWidgetItem(ConnNum));
    if (queryTermInfo && queryTermInfo->isValid()) {
        ui->tableTerm->item(currentRow, 1)->setData(Qt::UserRole, queryTermInfo->value("Term_ID"));
        ui->tableTerm->setItem(currentRow, 2, new QTableWidgetItem(queryTermInfo->value("TermDesc").toString()));
        QString testCost = queryTermInfo->value("TestCost").toString();
        if(testCost == "")testCost=cost;
        ui->tableTerm->setItem(currentRow, 3, new QTableWidgetItem(testCost));

        // 解析 TermPicPath 字段
        QString termPicPath = queryTermInfo->value("TermPicPath").toString();
        QStringList picInfoList = termPicPath.split("||");
        QString firstPicInfo = picInfoList.isEmpty() ? QString() : picInfoList.first();
        QStringList firstPicParts = firstPicInfo.split("*");
        QString imageFileName = firstPicParts.isEmpty() ? QString() : firstPicParts.first();
        QString strTagInfo = (firstPicParts.count() > 1) ? firstPicParts.last() : QString();
        if (!strTagInfo.isEmpty() && !imageFileName.isEmpty()) {
            ui->tableTerm->setItem(currentRow, 4, new QTableWidgetItem("是"));
            //从queryTermInfo->value("TermPicPath").toString()中解析出StrTagInfo，作为setData的内容。
            ui->tableTerm->item(currentRow, 4)->setData(Qt::UserRole, strTagInfo);
        } else {
            ui->tableTerm->setItem(currentRow, 4, new QTableWidgetItem("否"));
        }
        //从queryTermInfo->value("TermPicPath").toString()中解析出ImageFileName，调用QStringList findImagesInDir(const QDir &dir, QStringList &imageNames)获取图片的绝对路径
        //ui->tableTerm->setItem(currentRow, 5, new QTableWidgetItem(queryTermInfo->value("TermPicPath").toString()));
        QDir searchDir(QString(PIC_BASE_PATH) + "/" + supplier);
        QStringList foundImages = findImagesInDir(searchDir, QStringList() << imageFileName);
        QString absoluteImagePath = foundImages.isEmpty() ? QString() : foundImages.first();
        ui->tableTerm->setItem(currentRow, 5, new QTableWidgetItem(absoluteImagePath));

    } else {
        ui->tableTerm->setItem(currentRow, 2, new QTableWidgetItem(ConnDesc));
        ui->tableTerm->setItem(currentRow, 3, new QTableWidgetItem(cost));
        ui->tableTerm->setItem(currentRow, 4, new QTableWidgetItem("否"));
        ui->tableTerm->setItem(currentRow, 5, new QTableWidgetItem(""));
    }
}

void DialogUnitManage::on_RbFactory_clicked(bool checked)
{
    LoadDBFactory();
}

void DialogUnitManage::SetStackWidget(int PageIndex)
{
    ui->stackedWidget->setCurrentIndex(PageIndex);
}
void DialogUnitManage::on_BtnOK_clicked()
{
    Canceled=false;
    if(ui->tableWidgetSpur->currentRow()>=0) EquipmentTemplate_ID=ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),1)->data(Qt::UserRole).toInt();
    else EquipmentTemplate_ID=0;
    RetCode=1;
    close();
}

void DialogUnitManage::on_BtnCancel_clicked()
{
    Canceled=true;
    RetCode=0;
    close();
}


void DialogUnitManage::on_BtnSearch_clicked()
{
    //在table中检索
    if((ui->EdSearchUnitCode->text()=="")&&(ui->EdSearchUnitFactory->text()=="")&&(ui->EdSearchUnitName->text()=="")&&(ui->EdSearchUnitOrderNumber->text()=="")&&(ui->EdSearchUnitType->text()=="")) return;
    for(int i=0;i<ui->tableWidgetUnit->rowCount();i++)
    {
        if(ui->EdSearchUnitCode->text()!="")
            if(!ui->tableWidgetUnit->item(i,0)->text().contains(ui->EdSearchUnitCode->text())) ui->tableWidgetUnit->hideRow(i);
        if(ui->EdSearchUnitFactory->text()!="")
            if(!ui->tableWidgetUnit->item(i,4)->text().contains(ui->EdSearchUnitFactory->text())) ui->tableWidgetUnit->hideRow(i);
        if(ui->EdSearchUnitName->text()!="")
            if(!ui->tableWidgetUnit->item(i,2)->text().contains(ui->EdSearchUnitName->text())) ui->tableWidgetUnit->hideRow(i);
        if(ui->EdSearchUnitOrderNumber->text()!="")
            if(!ui->tableWidgetUnit->item(i,5)->text().contains(ui->EdSearchUnitOrderNumber->text())) ui->tableWidgetUnit->hideRow(i);
        if(ui->EdSearchUnitType->text()!="")
            if(!ui->tableWidgetUnit->item(i,1)->text().contains(ui->EdSearchUnitType->text())) ui->tableWidgetUnit->hideRow(i);
    }
}


void DialogUnitManage::on_BtnEditFunc_clicked()
{
    if(ui->tableWidgetSpur->currentRow()<0)
    {
        QMessageBox::information(this, "提示信息","未选择子块!", QMessageBox::Yes);
        return;
    }
    DialogFuncDefine *dlg=new DialogFuncDefine(this);
    dlg->SetCurrentIndex(ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),0)->data(Qt::UserRole).toString());//传递参数为功能ID
    if(ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),0)->data(Qt::UserRole).toString()=="")
    {
        dlg->SetVirtualPort(ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),7)->text(),ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),0)->text());
    }
    dlg->setModal(true);
    dlg->exec();
    if(dlg->Canceled) return;
    //更新table中的功能定义和数据库
    ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),0)->setText(dlg->FunctionDefineName);
    ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),0)->setData(Qt::UserRole,QVariant(dlg->FunctionDefineClass_ID));
    ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),7)->setText(dlg->FuncType);
    if(dlg->FuncType=="接线端口")
    {
        if(ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),2)->checkState()==Qt::Unchecked)
            ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),2)->setCheckState(Qt::Unchecked);
        if(ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),3)->checkState()==Qt::Unchecked)
            ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),3)->setCheckState(Qt::Unchecked);
    }
    else
    {
        ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->currentRow(),2,new QTableWidgetItem(""));
        ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->currentRow(),3,new QTableWidgetItem(""));
    }
}

//Lu ToDo 将器件信息保存至数据库
void DialogUnitManage::on_BtnApply_clicked()
{
    for(int i=0;i<ui->tableUnitDiagnosePara->rowCount();i++)
    {
        if(ui->tableUnitDiagnosePara->item(i,1)->text()=="")
        {
            QMessageBox::information(this, "提示信息","诊断参数名称不能为空!", QMessageBox::Yes);
            return;
        }
        if(ui->tableUnitDiagnosePara->item(i,3)->text()=="")
        {
            QMessageBox::information(this, "提示信息","诊断参数默认值不能为空!", QMessageBox::Yes);
            return;
        }
        if(!StrIsDouble(ui->tableUnitDiagnosePara->item(i,3)->text()))
        {
            QMessageBox::information(this, "提示信息","诊断参数默认值必须为数值!", QMessageBox::Yes);
            return;
        }
    }
    // 查看Class_ID
    QSqlQuery QueryClass(T_LibDatabase);
    QString classIDQueryStr = QString("SELECT * FROM Class WHERE Desc = '%1'").arg(ui->CbUnitGroup->currentText());
    QueryClass.exec(classIDQueryStr);
    if(!QueryClass.next()) {
        qDebug() << "No Class found for:" << ui->CbUnitGroup->currentText();
        return;
    }
    QString parentClassID = QueryClass.value("Class_ID").toString();
    classIDQueryStr = QString("SELECT * FROM Class WHERE Desc = '%1' AND ParentNo='%2'").arg(ui->CbUnitSubGroup->currentText(), parentClassID);
    QueryClass.exec(classIDQueryStr);
    if(!QueryClass.next()) {
        qDebug() << "No Sub-Class found for:" << ui->CbUnitSubGroup->currentText();
        return;
    }
    QString Class_ID = QueryClass.value("Class_ID").toString();

    QSqlQuery QueryFactory = QSqlQuery(T_LibDatabase);//设置数据库选择模型
    QString tempFactory = QString("SELECT Factory_ID FROM Factory WHERE Factory = '"+ui->CbUnitFactory->currentText()+"'");
    QueryFactory.exec(tempFactory);
    if(!QueryFactory.next()) return;
    QString Factory_ID=QueryFactory.value(0).toString();

    QSqlQuery QueryVar = QSqlQuery(T_LibDatabase);//设置数据库选择模型
    QString tempSQL=QString("UPDATE Equipment SET Factory_ID=:Factory_ID,Type=:Type,Spec=:Spec,PartCode=:PartCode,OrderNum=:OrderNum,Class_ID=:Class_ID,Name=:Name,Supplier=:Supplier,Desc=:Desc,Remark=:Remark,MultiLib=:MultiLib,Picture=:Picture,DataType=:DataType,TModel=:TModel,Structure=:Structure,RepairInfo=:RepairInfo,MTBF=:MTBF, PictureToAdd=:PictureToAdd "
                            "WHERE Equipment_ID='"+CurEquipment_ID+"'");
    QueryVar.prepare(tempSQL);
    QueryVar.bindValue(":Factory_ID",Factory_ID);
    QueryVar.bindValue(":Type",ui->EdUnitType->text());
    QueryVar.bindValue(":Spec",ui->EdUnitStandard->text());
    QueryVar.bindValue(":PartCode",ui->EdUnitCode->text());
    QueryVar.bindValue(":OrderNum",ui->EdOrderNumber->text());
    QueryVar.bindValue(":Class_ID",Class_ID);
    QueryVar.bindValue(":Name",ui->EdUnitName->text());
    QueryVar.bindValue(":Supplier",ui->CbUnitFactory->currentText());
    QueryVar.bindValue(":Desc",ui->TextEdDesc->document()->toPlainText());
    QueryVar.bindValue(":Remark",ui->EdInfo->text());
    QueryVar.bindValue(":MultiLib",ui->LbStampName->text());
    QueryVar.bindValue(":MTBF",ui->EdMTBF->text());

//    //Lu ToDo 保存器件图片信息至数据库==========
//    QString UnitJpgPath;
//    for(int i=0;i<ui->tableWidgetUnitPic->rowCount();i++)
//    {
//        if(UnitJpgPath!="") UnitJpgPath+="||";
//        UnitJpgPath+=ui->tableWidgetUnitPic->item(i,0)->data(Qt::UserRole).toString();
//    }
//    QueryVar.bindValue(":Picture",UnitJpgPath);
//    //==========保存器件图片信息至数据库==========
    // ToDo: 保存器件图片信息至数据库
    QStringList pictureList;
    QStringList pictureToAddList;
    for (int i = 0; i < ui->tableWidgetUnitPic->rowCount(); ++i) {
        QTableWidgetItem *item = ui->tableWidgetUnitPic->item(i, 0);
        if (!item) continue;

        QString imageName = QFileInfo(item->data(Qt::UserRole).toString()).fileName();
        if (item->background() == QColor(Qt::blue).lighter()) {
            pictureToAddList.append(imageName);
        } else {
            pictureList.append(imageName);
        }
    }
    qDebug()<<"pictureToAddList:"<<pictureToAddList;
    qDebug()<<"pictureList"<<pictureList;
    QueryVar.bindValue(":Picture", pictureList.join("||"));
    QueryVar.bindValue(":PictureToAdd", pictureToAddList.join("||"));
    // 保存器件图片信息至数据库结束

    QueryVar.bindValue(":DataType","");//ui->CbModuleType->currentText());
    QueryVar.bindValue(":TModel",QsciEdit->text());
    QString StrStructure;
    for(int i=0;i<ui->tableWidgetStructure->rowCount();i++)
    {
        if(i!=0) StrStructure+=";";
        StrStructure+=ui->tableWidgetStructure->item(i,0)->text()+","+((QComboBox *)ui->tableWidgetStructure->cellWidget(i,2))->currentText()+","+((QComboBox *)ui->tableWidgetStructure->cellWidget(i,3))->currentText();//+","+ui->tableWidgetStructure->item(i,4)->text();
    }
    QueryVar.bindValue(":Structure",StrStructure);
    QString StrRepairInfo;
    for(int i=0;i<ui->tableRepairInfo->rowCount();i++)
    {
        if(i!=0) StrRepairInfo+="￤￤";
        QString RepairPlan=ui->tableRepairInfo->item(i,2)->text();
        if(RepairPlan=="") RepairPlan="无";
        QString RepairResource=ui->tableRepairInfo->item(i,3)->text();
        if(RepairResource=="") RepairResource="无";
        StrRepairInfo+=ui->tableRepairInfo->item(i,0)->text()+"￤"+ui->tableRepairInfo->item(i,1)->text()+"￤"+RepairPlan+"￤"+RepairResource;
    }
    QueryVar.bindValue(":RepairInfo",StrRepairInfo);
    if (!QueryVar.exec()) {
        qDebug() << "Error executing Equipment update query:" << QueryVar.lastError().text();
    } else {
        qDebug() << "Equipment updated successfully for Equipment_ID:" << CurEquipment_ID;
    }

    //同步EquipmentTemplate表与tableWidgetSpur控件中内容
    QSqlQuery query(T_LibDatabase);
    // 收集tableWidgetSpur中所有的EquipmentTemplate_ID
    QStringList templateIDs;
    for (int i = 0; i < ui->tableWidgetSpur->rowCount(); i++) {
        QString id = ui->tableWidgetSpur->item(i, 1)->data(Qt::UserRole).toString();
        if (!id.isEmpty()) {
            templateIDs << id;
        }
    }
    //查询并收集要删除的 EquipmentTemplate 记录的ID：
    QStringList deleteTemplateIDs;
    QString selectDeleteTemplateSql = QString("SELECT EquipmentTemplate_ID FROM EquipmentTemplate WHERE Equipment_ID = %1 AND EquipmentTemplate_ID NOT IN (%2)").arg(CurEquipment_ID).arg(templateIDs.join(", "));
    QSqlQuery selectQuery(T_LibDatabase);
    if (!selectQuery.exec(selectDeleteTemplateSql)) {
        qDebug() << "Error selecting old EquipmentTemplate IDs for deletion:" << selectQuery.lastError().text();
    } else {
        while(selectQuery.next()) {
            deleteTemplateIDs.append(selectQuery.value(0).toString());
        }
    }
    // 删除TermInfo记录
    if (!deleteTemplateIDs.isEmpty()) {
        QString deleteTermInfoSql = QString("DELETE FROM TermInfo WHERE EquipmentTemplate_ID IN (%1)").arg(deleteTemplateIDs.join(", "));
        if (!query.exec(deleteTermInfoSql)) {
            qDebug() << "Error deleting old TermInfo records:" << query.lastError().text();
        } else {
            int numRows = query.numRowsAffected();
            qDebug() << "Deleted" << numRows << "old TermInfo records for corresponding EquipmentTemplate_IDs";
        }
    }
    // 删除EquipmentTemplate记录
    QString deleteTemplateSql = QString("DELETE FROM EquipmentTemplate WHERE EquipmentTemplate_ID IN (%1)").arg(deleteTemplateIDs.join(", "));
    if (!query.exec(deleteTemplateSql)) {
        qDebug() << "Error deleting old EquipmentTemplate records:" << query.lastError().text();
    } else {
        int numRows = query.numRowsAffected();
        qDebug() << "Deleted" << numRows << "old EquipmentTemplate records";
    }
    //    // 删除EquipmentTemplate记录
    //    QString deleteTemplateSql = QString("DELETE FROM EquipmentTemplate WHERE Equipment_ID = %1 AND EquipmentTemplate_ID NOT IN (%2)").arg(CurEquipment_ID).arg(templateIDs.join(", "));
    //    if (!query.exec(deleteTemplateSql)) {
    //        qDebug() << "Error deleting old EquipmentTemplate records:" << query.lastError().text();
    //    } else {
    //        int numRows = query.numRowsAffected();
    //        qDebug() << "Deleted" << numRows << "old EquipmentTemplate records for Equipment_ID:" << CurEquipment_ID;
    //    }
    //    // 删除TermInfo记录
    //    QString deleteTermInfoSql = QString("DELETE FROM TermInfo WHERE Equipment_ID = %1 AND EquipmentTemplate_ID NOT IN (%2)").arg(CurEquipment_ID).arg(templateIDs.join(", "));
    //    if (!query.exec(deleteTermInfoSql)) {
    //        qDebug() << "Error deleting old TermInfo records:" << query.lastError().text();
    //    } else {
    //        int numRows = query.numRowsAffected();
    //        qDebug() << "Deleted" << numRows << "old TermInfo records for Equipment_ID:" << CurEquipment_ID;
    //    }

    for (int i = 0; i < ui->tableWidgetSpur->rowCount(); i++) {
        // 检索FunctionDefineCode
        QString functionDefineClassID = ui->tableWidgetSpur->item(i, 0)->data(Qt::UserRole).toString();
        QSqlQuery QueryFunctionDefine(T_LibDatabase);
        QueryFunctionDefine.exec("SELECT FunctionDefineCode FROM FunctionDefineClass WHERE FunctionDefineClass_ID = '" + functionDefineClassID + "'");
        QString FunDefine;
        if (QueryFunctionDefine.next()) {
            FunDefine = QueryFunctionDefine.value(0).toString();
        }

        // 检查是否需要插入新记录
        QString equipmentTemplateId = ui->tableWidgetSpur->item(i, 1)->data(Qt::UserRole).toString();
        bool isExistingTemplateId = !equipmentTemplateId.isEmpty();

        if (isExistingTemplateId) {
            query.prepare("UPDATE EquipmentTemplate SET "
                          "FunDefine = :FunDefine, Symbol = :Symbol, ConnNum = :ConnNum, "
                          "SpurDT = :SpurDT, Designation = :Designation, FuncType = :FuncType, "
                          "SourceConn = :SourceConn, ExecConn = :ExecConn, SourcePrior = :SourcePrior, "
                          "TestCost = :TestCost, InterConnect = :InterConnect "
                          "WHERE EquipmentTemplate_ID = :EquipmentTemplate_ID");
        } else {
            equipmentTemplateId = QString::number(GetMaxIDOfLibDatabase(T_LibDatabase, "EquipmentTemplate", "EquipmentTemplate_ID"));
            query.prepare("INSERT INTO EquipmentTemplate "
                          "(EquipmentTemplate_ID, Equipment_ID, FunDefine, Symbol, ConnNum, "
                          "SpurDT, Designation, FuncType, SourceConn, ExecConn, SourcePrior, TestCost, InterConnect) "
                          "VALUES "
                          "(:EquipmentTemplate_ID, :Equipment_ID, :FunDefine, :Symbol, :ConnNum, "
                          ":SpurDT, :Designation, :FuncType, :SourceConn, :ExecConn, :SourcePrior, :TestCost, :InterConnect)");
        }
        query.bindValue(":EquipmentTemplate_ID", equipmentTemplateId);
        query.bindValue(":Equipment_ID", CurEquipment_ID);
        query.bindValue(":FunDefine", FunDefine.isEmpty() ? ui->tableWidgetSpur->item(i, 0)->text() : FunDefine);
        query.bindValue(":Symbol", ui->tableWidgetSpur->item(i, 5)->text());
        query.bindValue(":ConnNum", ui->tableWidgetSpur->item(i, 1)->text());
        query.bindValue(":SourceConn", ui->tableWidgetSpur->item(i, 2)->checkState() == Qt::Checked);
        query.bindValue(":ExecConn", ui->tableWidgetSpur->item(i, 3)->checkState() == Qt::Checked);
        query.bindValue(":SpurDT", ui->tableWidgetSpur->item(i, 4)->text());
        query.bindValue(":Designation", ui->tableWidgetSpur->item(i, 6)->text());
        query.bindValue(":FuncType", ui->tableWidgetSpur->item(i, 7)->text());
        query.bindValue(":SourcePrior", ui->tableWidgetSpur->item(i, 2)->text());
        query.bindValue(":TestCost", ui->tableWidgetSpur->item(i, 8)->text());

        QString interConnect;
        if (StrIsNumber(ui->tableWidgetSpur->item(i, 9)->text())) {
            int RowVal = ui->tableWidgetSpur->item(i, 9)->text().toInt();
            if (RowVal <= ui->tableWidgetSpur->rowCount()) {
                interConnect = ui->tableWidgetSpur->item(RowVal - 1, 1)->data(Qt::UserRole).toString();
            }
        }
        query.bindValue(":InterConnect", interConnect);

        if (!query.exec()) {
            qDebug() << "Error executing SQL query:" << query.lastError().text();
        }
    }

    tempSQL="DELETE FROM EquipmentDiagnosePara WHERE Equipment_ID='"+CurEquipment_ID+"'";
    QueryVar.exec(tempSQL);
    for(int i=0;i<ui->tableUnitDiagnosePara->rowCount();i++)
    {
        int DiagnoseParaID=GetMaxIDOfDB(T_LibDatabase,"EquipmentDiagnosePara","DiagnoseParaID");
        tempSQL = "INSERT INTO EquipmentDiagnosePara (DiagnoseParaID,Equipment_ID,Name,Unit,DefaultValue,Remark)"
                  "VALUES (:DiagnoseParaID,:Equipment_ID,:Name,:Unit,:DefaultValue,:Remark)";
        QueryVar.prepare(tempSQL);
        QueryVar.bindValue(":DiagnoseParaID",DiagnoseParaID);
        QueryVar.bindValue(":Equipment_ID",CurEquipment_ID);
        QueryVar.bindValue(":Name",ui->tableUnitDiagnosePara->item(i,1)->text());
        QueryVar.bindValue(":Unit",ui->tableUnitDiagnosePara->item(i,2)->text());
        QueryVar.bindValue(":DefaultValue",ui->tableUnitDiagnosePara->item(i,3)->text());
        QueryVar.bindValue(":Remark",ui->tableUnitDiagnosePara->item(i,4)->text());
        QueryVar.exec();
    }
    //更新tableWidgetUnit
    ui->tableWidgetUnit->item(ui->tableWidgetUnit->currentRow(),0)->setText(ui->EdUnitCode->text());//编号
    ui->tableWidgetUnit->item(ui->tableWidgetUnit->currentRow(),1)->setText(ui->EdUnitType->text());//型号
    ui->tableWidgetUnit->item(ui->tableWidgetUnit->currentRow(),3)->setText(ui->EdUnitStandard->text());//规格
    ui->tableWidgetUnit->item(ui->tableWidgetUnit->currentRow(),2)->setText(ui->EdUnitName->text());//名称
    ui->tableWidgetUnit->item(ui->tableWidgetUnit->currentRow(),4)->setText(ui->CbUnitFactory->currentText());//厂家
    ui->tableWidgetUnit->item(ui->tableWidgetUnit->currentRow(),5)->setText(ui->EdOrderNumber->text());//订货号
    ui->tableWidgetUnit->item(ui->tableWidgetUnit->currentRow(),6)->setText(ui->TextEdDesc->document()->toPlainText());//描述
    ui->tableWidgetUnit->item(ui->tableWidgetUnit->currentRow(),7)->setText(ui->EdInfo->text());//备注
    on_tableWidgetUnit_clicked(ui->tableWidgetUnit->currentIndex());
    emit(SignalUpdated());
}

void DialogUnitManage::on_BtnAddUnitPic_clicked()
{
    QFileDialog fileDialog(this);
    fileDialog.setWindowTitle(tr("选择文件"));
    fileDialog.setDirectory(QString(PIC_BASE_PATH));
    fileDialog.setNameFilter(tr("Images (*.png *.jpg *.jpeg *.gif *.webp *.bmp *.xpm )"));
    // fileDialog->setFileMode(QFileDialog::ExistingFiles);
    fileDialog.setViewMode(QFileDialog::Detail);
    if (!fileDialog.exec()) return;
    QStringList fileNames=fileDialog.selectedFiles();
    if(fileNames.count()!=1)
    {
        QMessageBox::warning(nullptr, "提示", "请选择一张图片！");
        return;
    }
    QFileInfo SelectedFilePath(fileNames.at(0));
    ui->tableWidgetUnitPic->setRowCount(ui->tableWidgetUnitPic->rowCount()+1);
    ui->tableWidgetUnitPic->setItem(ui->tableWidgetUnitPic->rowCount()-1,0,new QTableWidgetItem(SelectedFilePath.fileName()));
    ui->tableWidgetUnitPic->item(ui->tableWidgetUnitPic->rowCount()-1,0)->setData(Qt::UserRole,fileNames.at(0));
    on_tableWidgetUnitPic_clicked(ui->tableWidgetUnitPic->model()->index(ui->tableWidgetUnitPic->rowCount()-1,0));
}

void DialogUnitManage::on_BtnReplaceUnitPic_clicked()
{
    if(ui->tableWidgetUnitPic->currentRow()<0)
    {
        QMessageBox::warning(nullptr, "提示", "请选择一条图片记录！");
        return;
    }
    QFileDialog fileDialog(this);
    fileDialog.setWindowTitle(tr("选择文件"));
    fileDialog.setDirectory(QString(PIC_BASE_PATH));
    fileDialog.setNameFilter(tr("Images (*.png *.jpg *.jpeg *.gif *.webp *.bmp *.xpm )"));
    // fileDialog->setFileMode(QFileDialog::ExistingFiles);
    fileDialog.setViewMode(QFileDialog::Detail);
    if (!fileDialog.exec()) return;
    QStringList fileNames=fileDialog.selectedFiles();
    if(fileNames.count()!=1)
    {
        QMessageBox::warning(nullptr, "提示", "请选择一张图片！");
        return;
    }
    QFileInfo SelectedFilePath(fileNames.at(0));
    ui->tableWidgetUnitPic->setItem(ui->tableWidgetUnitPic->currentRow(),0,new QTableWidgetItem(SelectedFilePath.fileName()));
    ui->tableWidgetUnitPic->item(ui->tableWidgetUnitPic->currentRow(),0)->setData(Qt::UserRole,fileNames.at(0));
    on_tableWidgetUnitPic_clicked(ui->tableWidgetUnitPic->model()->index(ui->tableWidgetUnitPic->currentRow(),0));
}

void DialogUnitManage::on_BtnDelUnitPic_clicked()
{
    if(ui->tableWidgetUnitPic->currentRow() < 0) {QMessageBox::warning(this, "提示", "请选择一条图片记录！");return;}
    QTableWidgetItem* selected_item = ui->tableWidgetUnitPic->item(ui->tableWidgetUnitPic->currentRow(), 0);
    if(selected_item)
    {
        auto response = QMessageBox::question(this, "确认删除", "您确定要删除对应的文件吗？", QMessageBox::Yes | QMessageBox::No);
        if(response == QMessageBox::Yes)
        {
            QSqlQuery query(T_LibDatabase);
            query.exec("BEGIN TRANSACTION;"); // 开始事务
            // 查询包含该图片的设备记录
            QString selectSql = QString("SELECT PictureToAdd FROM Equipment WHERE Equipment_ID = '%1'").arg(CurEquipment_ID);
            query.exec(selectSql);
            if(query.next())
            {
                QStringList pictures = query.value("PictureToAdd").toString().split("||");
                pictures.removeAll(selected_item->text()); // 从列表中移除该图片路径
                QString newPicturePath = pictures.join("||");
                qDebug()<<"newPicturePath:"<<newPicturePath;
                // 更新设备记录
                QString updateSql = QString("UPDATE Equipment SET PictureToAdd = '%1' WHERE Equipment_ID = '%2'").arg(newPicturePath, CurEquipment_ID);
                if(!query.exec(updateSql))
                {
                    QMessageBox::warning(this, "错误", "更新数据库失败：" + query.lastError().text());
                    query.exec("ROLLBACK;"); // 回滚事务
                    return;
                }
            }
            else return;
            QString file_path = selected_item->data(Qt::UserRole).toString();
            QFile file(file_path);
            if(!file.remove()) QMessageBox::warning(this, "错误", "无法删除文件：" + file.errorString());
            else query.exec("COMMIT;"); // 提交事务
        }
    }
    ui->tableWidgetUnitPic->removeRow(ui->tableWidgetUnitPic->currentRow());
    m_scene_unit.SetBackGroundImage(QPixmap(""));
}

void DialogUnitManage::on_BtnAckUnitPic_clicked()
{
    int currentRow = ui->tableWidgetUnitPic->currentRow();
    if (currentRow < 0) return;

    QTableWidgetItem *item = ui->tableWidgetUnitPic->item(currentRow, 0);
    if (item != nullptr) {
        item->setBackground(Qt::transparent);  // 设置背景色为透明
    }
}

void DialogUnitManage::on_BtnReplaceStamp_clicked()
{
    QFileDialog fileDialog(this);
    fileDialog.setWindowTitle(tr("选择文件"));
    fileDialog.setDirectory("C:/TBD/UserData/MultiLib");
    fileDialog.setNameFilter(tr("dwg(*.dwg)"));
    // fileDialog->setFileMode(QFileDialog::ExistingFiles);
    fileDialog.setViewMode(QFileDialog::Detail);
    if (!fileDialog.exec()) return;
    QStringList fileNames=fileDialog.selectedFiles();
    QPixmap p=QPixmap(fileNames.at(0));

    QFileInfo file(fileNames.at(0));
    QString StampJpgName=file.fileName();
    StampJpgName.replace("dwg","jpg");
    UnitSymbolsView(fileNames.at(0),"C:/TBD/data/UserData/MultiLibJpg/"+StampJpgName,ui->LbStampJpg,true);
    ui->LbStampName->setText(file.fileName());
}

void DialogUnitManage::on_BtnDeleteStamp_clicked()
{
    QPixmap p=QPixmap("");
    ui->LbStampJpg->setPixmap(p.scaled(ui->LbStampJpg->width(),ui->LbStampJpg->height()));
    ui->LbStampName->setText("");
}

void DialogUnitManage::on_BtnCopySelectRow_clicked()
{
    ui->tableWidgetSpur->setRowCount(ui->tableWidgetSpur->rowCount()+1);
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,0,new QTableWidgetItem(ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),0)->text()));
    ui->tableWidgetSpur->item(ui->tableWidgetSpur->rowCount()-1,0)->setFlags(ui->tableWidgetSpur->item(ui->tableWidgetSpur->rowCount()-1,0)->flags()&(~Qt::ItemIsEditable));
    ui->tableWidgetSpur->item(ui->tableWidgetSpur->rowCount()-1,0)->setData(Qt::UserRole,ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),0)->data(Qt::UserRole));
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,1,new QTableWidgetItem(ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),1)->text()));
    QTableWidgetItem *ItemSourceConn = new QTableWidgetItem(ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),2)->text());
    if(ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),2)->checkState()==Qt::Checked) ItemSourceConn->setCheckState(Qt::Checked);
    else
    {
        if((ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),7)->text()=="")||(ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),7)->text()=="接线端口"))
            ItemSourceConn->setCheckState(Qt::Unchecked);
    }
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,2,ItemSourceConn);

    QTableWidgetItem *ItemExecConn = new QTableWidgetItem("");
    if(ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),3)->checkState()==Qt::Checked) ItemExecConn->setCheckState(Qt::Checked);
    else
    {
        if((ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),7)->text()=="")||(ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),7)->text()=="接线端口"))
            ItemExecConn->setCheckState(Qt::Unchecked);
    }
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,3,ItemExecConn);


    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,4,new QTableWidgetItem(ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),4)->text()));
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,5,new QTableWidgetItem(ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),5)->text()));
    ui->tableWidgetSpur->item(ui->tableWidgetSpur->rowCount()-1,5)->setFlags(ui->tableWidgetSpur->item(ui->tableWidgetSpur->rowCount()-1,5)->flags()&(~Qt::ItemIsEditable));
    ui->tableWidgetSpur->item(ui->tableWidgetSpur->rowCount()-1,5)->setData(Qt::UserRole,QVariant(0));
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,6,new QTableWidgetItem(ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),6)->text()));
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,7,new QTableWidgetItem(ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),7)->text()));
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,8,new QTableWidgetItem(ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),8)->text()));
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,9,new QTableWidgetItem(ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),9)->text()));
}

void DialogUnitManage::on_BtnAddSpur_clicked()
{
    //打开功能选择窗口
    dlgFuncDefine->Canceled=true;
    dlgFuncDefine->setModal(true);
    dlgFuncDefine->exec();
    if(dlgFuncDefine->Canceled) return;
    //更新table中的功能定义和数据库
    ui->tableWidgetSpur->setRowCount(ui->tableWidgetSpur->rowCount()+1);
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,0,new QTableWidgetItem(dlgFuncDefine->FunctionDefineName));
    ui->tableWidgetSpur->item(ui->tableWidgetSpur->rowCount()-1,0)->setFlags(ui->tableWidgetSpur->item(ui->tableWidgetSpur->rowCount()-1,0)->flags()&(~Qt::ItemIsEditable));
    ui->tableWidgetSpur->item(ui->tableWidgetSpur->rowCount()-1,0)->setData(Qt::UserRole,QVariant(dlgFuncDefine->FunctionDefineClass_ID));
    //根据选择的功能是几个Term决定
    QString ConnNum;
    QSqlQuery QueryVar = QSqlQuery(T_LibDatabase);//设置数据库选择模型
    QString temp = "SELECT * FROM FunctionDefineClass WHERE FunctionDefineClass_ID = '"+dlgFuncDefine->FunctionDefineClass_ID+"'";//SPSFuncDef WHERE FuncDef = '"+dlgFuncDefine->FunctionDefineClass_ID+"'";
    QueryVar.exec(temp);
    if(QueryVar.next())//
    {
        for(int i=0;i<GetDwgTermCount("C:/TBD/SPS/"+QueryVar.value("DefaultSymbol").toString()+".dwg",QueryVar.value("DefaultSymbol").toString());i++)
        {
            if(i>0) ConnNum+="￤";
            ConnNum+=QString::number(i+1);
        }
    }
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,1,new QTableWidgetItem(ConnNum));
    //ui->tableWidgetSpur->item(ui->tableWidgetSpur->rowCount()-1,1)->setData(Qt::UserRole,QVariant(0)); //新增的功能子块的ui->tableWidgetSpur->item(,1):EquipmentTemplate_ID为空
    qDebug()<<"dlgFuncDefine->FuncType="<<dlgFuncDefine->FuncType;
    QTableWidgetItem *ItemSourceConn = new QTableWidgetItem("");
    if(dlgFuncDefine->FuncType=="接线端口") ItemSourceConn->setCheckState(Qt::Unchecked);
    //ItemSourceConn->setFlags(ItemSourceConn->flags()&(~Qt::ItemIsEditable));
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,2,ItemSourceConn);

    QTableWidgetItem *ItemExecConn = new QTableWidgetItem("");
    if(dlgFuncDefine->FuncType=="接线端口") ItemExecConn->setCheckState(Qt::Unchecked);
    ItemExecConn->setFlags(ItemExecConn->flags()&(~Qt::ItemIsEditable));
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,3,ItemExecConn);

    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,4,new QTableWidgetItem(""));
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,5,new QTableWidgetItem("")); //符号
    ui->tableWidgetSpur->item(ui->tableWidgetSpur->rowCount()-1,5)->setFlags(ui->tableWidgetSpur->item(ui->tableWidgetSpur->rowCount()-1,5)->flags()&(~Qt::ItemIsEditable));
    ui->tableWidgetSpur->item(ui->tableWidgetSpur->rowCount()-1,5)->setData(Qt::UserRole,QVariant(0));
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,6,new QTableWidgetItem(""));
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,7,new QTableWidgetItem(dlgFuncDefine->FuncType));//功能类型
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,8,new QTableWidgetItem(""));//测试代价
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->rowCount()-1,9,new QTableWidgetItem(""));//受控
}

void DialogUnitManage::on_BtnDeleteSpur_clicked()
{
    if(ui->tableWidgetSpur->currentRow()<0)
    {
        QMessageBox::information(this, "提示信息","未选择子块!", QMessageBox::Yes);
        return;
    }
    ui->tableWidgetSpur->removeRow(ui->tableWidgetSpur->currentRow());
}

void DialogUnitManage::LocateToUnitIndex(QString Equipment_ID)
{

}

void DialogUnitManage::on_BtnAddUnit_clicked()
{
    if(!ui->treeViewUnitGroup->currentIndex().isValid()) return;
    QSqlQuery QueryEquipment = QSqlQuery(T_LibDatabase);//设置数据库选择模型
    QString SqlStr = QString("INSERT INTO Equipment (Equipment_ID,Factory_ID,PartCode,Class_ID,DataType,TModel,Structure,CodeUpdatedTime)"
                             "VALUES (:Equipment_ID,:Factory_ID,:PartCode,:Class_ID,:DataType,:TModel,:Structure,:CodeUpdatedTime)");
    QueryEquipment.prepare(SqlStr);
    int MaxEquipment_ID=GetMaxIDOfLibDatabase(T_LibDatabase,"Equipment","Equipment_ID");
    QueryEquipment.bindValue(":Equipment_ID",QString::number(MaxEquipment_ID));
    if(ui->RbFactory->isChecked())
    {
        QueryEquipment.bindValue(":Factory_ID",ui->treeViewUnitGroup->currentIndex().parent().parent().data(Qt::UserRole).toString());
    }
    else QueryEquipment.bindValue(":Factory_ID","1400");
    QueryEquipment.bindValue(":PartCode",QString::number(MaxEquipment_ID));
    QueryEquipment.bindValue(":Class_ID",ui->treeViewUnitGroup->currentIndex().data(Qt::UserRole).toString());
    QueryEquipment.bindValue(":DataType","");//ui->CbModuleType->currentText());
    QueryEquipment.bindValue(":TModel","");
    QueryEquipment.bindValue(":Structure","");
    QueryEquipment.bindValue(":CodeUpdatedTime",QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    QueryEquipment.exec();
    on_treeViewUnitGroup_clicked(ui->treeViewUnitGroup->currentIndex());
}

void DialogUnitManage::on_tableWidgetSpur_clicked(const QModelIndex &index)
{
    if(!index.isValid()) ui->BtnCopySelectRow->setEnabled(false);
    else ui->BtnCopySelectRow->setEnabled(true);
}

void DialogUnitManage::on_tableWidgetSpur_doubleClicked(const QModelIndex &index)
{
    if(!index.isValid()) return;
    if(index.column()==5)
    {
        on_BtnSpurSymbolEdit_clicked();
    }
}

void DialogUnitManage::on_BtnSpurSymbolEdit_clicked()
{
    dlgLoadSymbol->SetCurStackedWidgetIndex(1);
    dlgLoadSymbol->Canceled=true;
    dlgLoadSymbol->move(this->width()-dlgLoadSymbol->width()-20,50);
    dlgLoadSymbol->setModal(true);
    dlgLoadSymbol->show();
    dlgLoadSymbol->exec();
    if(dlgLoadSymbol->Canceled) return;
    if(dlgLoadSymbol->RetCode!=3)  return;//载入当前符号
    //查看选择符号的端号数量与功能定义是否一致
    QSqlQuery QuerySymb2Lib = QSqlQuery(T_LibDatabase);//设置数据库选择模型
    QString SqlStr = "SELECT * FROM Symb2Lib WHERE Symb2Lib_ID = '"+dlgLoadSymbol->SymbolID+"'";
    QuerySymb2Lib.exec(SqlStr);
    if(QuerySymb2Lib.next())
    {
        //2023.3.18 xcc 待修改
        //if(QuerySymb2Lib.value("TermCount").toInt()!=ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),5)->text().split("￤").count())
        {
            //QMessageBox::information(this,"提示信息","所选图形端号数量与端号列数量不一致，请重新选择！");
            //return;
        }
    }
    ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),5)->setText(dlgLoadSymbol->BlockFileName.mid(0,dlgLoadSymbol->BlockFileName.count()-4));

}

void DialogUnitManage::on_BtnFactoryManage_clicked()
{
    DialogFactoryManage *Dlg=new DialogFactoryManage(this);
    Dlg->setModal(true);
    Dlg->show();
    Dlg->exec();
    delete Dlg;
    UpdateCbFactory();
    if(ui->RbFactory->isChecked())
    {
        LoadDBFactory();
    }
    else LoadDBGroup();
}

void DialogUnitManage::on_BtnCopyUnit_clicked()
{
    if(ui->tableWidgetUnit->currentRow()<0) return;
    if(ui->treeViewUnitGroup->currentIndex().isValid())
    {
        if(ui->treeViewUnitGroup->currentIndex().data(Qt::WhatsThisRole).toString()=="2") ui->BtnPasteUnit->setEnabled(true);
    }
    CopyEquipment_ID=ui->tableWidgetUnit->item(ui->tableWidgetUnit->currentRow(),0)->data(Qt::UserRole).toString();
}

void DialogUnitManage::on_BtnPasteUnit_clicked()
{
    if(!ui->treeViewUnitGroup->currentIndex().isValid()) return;
    if(ui->treeViewUnitGroup->currentIndex().data(Qt::WhatsThisRole).toString()!="2") return;

    T_LibDatabase.transaction(); // 开始事务
    QSqlQuery query(T_LibDatabase);

    QString newEquipment_Id = "0";
    newEquipment_Id=QString::number(GetMaxIDOfLibDatabase(T_LibDatabase,"Equipment","Equipment_ID"));

    // 定义SQL语句以复制并更新Equipment记录
    QString SqlStr = QString(
                "INSERT INTO Equipment ("
                "    Equipment_ID, Factory_ID, Series_ID, Type, Spec, PartCode, OrderNum, "
                "    ErpCode, Class_ID, Name, Name2, Name3, Supplier, Desc, Remark, Hyperlink, "
                "    HaltProduction, MultiLib, Part2Lib, OverviewLib, Picture, HoleLib, LayLib, "
                "    StructureLib, ModelLib, DataType, TModel, Structure, RepairInfo, "
                "    CodeUpdatedTime, SymbolUpdatedTime, MTBF"
                ") "
                "SELECT "
                "    %1, Factory_ID, Series_ID, Type, Spec, "
                "    PartCode || '-副本', OrderNum, ErpCode, '%2', Name, Name2, Name3, Supplier, "
                "    Desc, Remark, Hyperlink, HaltProduction, MultiLib, Part2Lib, OverviewLib, "
                "    Picture, HoleLib, LayLib, StructureLib, ModelLib, DataType, TModel, Structure, "
                "    RepairInfo, CodeUpdatedTime, SymbolUpdatedTime, MTBF "
                "FROM Equipment "
                "WHERE Equipment_ID = '%3'"
                ).arg(newEquipment_Id, ui->treeViewUnitGroup->currentIndex().data(Qt::UserRole).toString(), CopyEquipment_ID);

    // 执行SQL语句
    if (!query.exec(SqlStr)) {
        qDebug() << "Update query failed for Equipment_ID" << CurEquipment_ID << ":" << query.lastError().text();
        QMessageBox::warning(nullptr, "提示", "复制元件失败：" + query.lastError().text());
        T_LibDatabase.rollback(); // 回滚事务
        return;
    }

    SqlStr = "SELECT * FROM EquipmentTemplate WHERE Equipment_ID = '"+CopyEquipment_ID+"'";
    QSqlQuery QuerySearch = QSqlQuery(T_LibDatabase);//设置数据库选择模型
    QuerySearch.exec(SqlStr);
    while(QuerySearch.next())
    {
        QString newEquipmentTemplate_ID=QString::number(GetMaxIDOfLibDatabase(T_LibDatabase,"EquipmentTemplate","EquipmentTemplate_ID"));
        QString curEquipmentTemplate_ID=QuerySearch.value("EquipmentTemplate_ID").toString();
        SqlStr = QString(
                    "INSERT INTO EquipmentTemplate ("
                    "    EquipmentTemplate_ID,Equipment_ID, FunDefine, FuncType, CommonAttr1, "
                    "    FunStatus, IntrinsicSafe, Symbol, Desc, Bus, ConnNum, ConnDesc, "
                    "    ConnArea, ConnAbility, idx, TechnicalParam, Indexstartaddress, "
                    "    Designation, SourceConn, ExecConn, SourcePrior, InterConnect, "
                    "    TestCost, Picture, SpurDT"
                    ") "
                    "SELECT "
                    "    %1, %2, FunDefine, FuncType, CommonAttr1, "
                    "    FunStatus, IntrinsicSafe, Symbol, Desc, Bus, ConnNum, ConnDesc, "
                    "    ConnArea, ConnAbility, idx, TechnicalParam, Indexstartaddress, "
                    "    Designation, SourceConn, ExecConn, SourcePrior, InterConnect, "
                    "    TestCost, Picture, SpurDT "
                    "FROM EquipmentTemplate WHERE EquipmentTemplate_ID = %3"
                    ).arg(newEquipmentTemplate_ID,newEquipment_Id,curEquipmentTemplate_ID);

        // 执行 SQL 插入语句
        if (!query.exec(SqlStr)) {
            qDebug() << "复制设备模板记录失败：" << CurEquipment_ID << ":" << query.lastError().text();
            T_LibDatabase.rollback(); // 回滚事务
            QMessageBox::warning(nullptr, "错误", "复制设备模板记录失败：" + query.lastError().text());
            return;
        }

        //将数据库TermInfo表中Equipment_ID==CopyEquipment_ID且EquipmentTemplate_ID==curEquipmentTemplate_ID的行复制一份，且将Equipment_ID改为newEquipment_Id，EquipmentTemplate_ID改为newEquipmentTemplate_ID
        // 复制 TermInfo 表记录
        SqlStr = QString(
                    "INSERT INTO TermInfo ("
                    "    Equipment_ID, EquipmentTemplate_ID, TermNum, TestCost, TermPicPath, "
                    "    TagType, TagPos, TagEdge, TagColor"
                    ") "
                    "SELECT "
                    "    %1, %2, TermNum, TestCost, TermPicPath, "
                    "    TagType, TagPos, TagEdge, TagColor "
                    "FROM TermInfo "
                    "WHERE Equipment_ID = '%3' AND EquipmentTemplate_ID = %4"
                    ).arg(newEquipment_Id, newEquipmentTemplate_ID, CopyEquipment_ID, curEquipmentTemplate_ID);

        if (!query.exec(SqlStr)) {
            qDebug() << "复制 TermInfo 记录失败：" << query.lastError().text();
            T_LibDatabase.rollback(); // 回滚事务
            QMessageBox::warning(nullptr, "错误", "复制 TermInfo 记录失败：" + query.lastError().text());
            return;
        }
    }

    T_LibDatabase.commit(); // 提交事务
    on_treeViewUnitGroup_clicked(ui->treeViewUnitGroup->currentIndex());
    ui->tableWidgetUnit->setCurrentIndex(ui->tableWidgetUnit->model()->index(ui->tableWidgetUnit->rowCount()-1,0));
    on_tableWidgetUnit_clicked(ui->tableWidgetUnit->currentIndex());
}

void DialogUnitManage::on_BtnDeleteUnit_clicked()
{
    QList<QTableWidgetSelectionRange> selectedRanges = ui->tableWidgetUnit->selectedRanges();
    if(selectedRanges.isEmpty()) return;

    QStringList equipmentIDs;
    foreach (const QTableWidgetSelectionRange &range, selectedRanges) {
        for (int row = range.topRow(); row <= range.bottomRow(); ++row) {
            QTableWidgetItem *item = ui->tableWidgetUnit->item(row, 0);
            if (item) {
                equipmentIDs.append(item->data(Qt::UserRole).toString());
            }
        }
    }

    int selectedRows = equipmentIDs.size();
    if(selectedRows == 0) return;

    QSqlQuery query(T_LibDatabase);
    query.exec("BEGIN TRANSACTION;"); // 开始事务

    QString ids = "'" + equipmentIDs.join("','") + "'";

    // 删除EquipmentTemplate记录，并获取删除的EquipmentTemplate_ID
    QStringList deleteTemplateIDs;
    QString sqlStr = "SELECT EquipmentTemplate_ID FROM EquipmentTemplate WHERE Equipment_ID IN (" + ids + ")";
    if (!query.exec(sqlStr)) {
        qDebug() << "Error selecting old EquipmentTemplate IDs for deletion:" << query.lastError().text();
        query.exec("ROLLBACK;");
        return;
    }
    while(query.next()) {
        deleteTemplateIDs.append(query.value(0).toString());
    }

    QString templateIds = deleteTemplateIDs.join(", ");

    sqlStr = "DELETE FROM TermInfo WHERE Equipment_ID IN (" + ids + ") OR EquipmentTemplate_ID IN (" + templateIds + ")";
    bool success = query.exec(sqlStr);

    if(success) {
        sqlStr = "DELETE FROM EquipmentTemplate WHERE EquipmentTemplate_ID IN (" + templateIds + ")";
        success = query.exec(sqlStr);
    }

    int deletedCount = 0;
    if(success) {
        sqlStr = "DELETE FROM Equipment WHERE Equipment_ID IN (" + ids + ")";
        success = query.exec(sqlStr);
        deletedCount = query.numRowsAffected();
    }

    if(success)
        query.exec("COMMIT;"); // 提交事务
    else
        query.exec("ROLLBACK;"); // 回滚事务

    int errorCount = selectedRows - deletedCount;

    // 显示消息框
    QMessageBox::information(this, "删除结果", QString("选中了%1条记录，成功删除%2条记录，发生错误数：%3").arg(selectedRows).arg(deletedCount).arg(errorCount));
}



void DialogUnitManage::InitTEdit()
{
    QsciEdit = new QsciScintilla();
    ui->TEditLayout->addWidget(QsciEdit);
    ui->frame_Edit->setLayout(ui->TEditLayout);
    //connect(QsciEdit, SIGNAL(textChanged()),this, SLOT(ModelWasModified()));
    //setCurrentFile("");
    //设置字体
    QFont font("Courier", 10, QFont::Normal);
    QsciEdit->setFont(font);
    QsciEdit->setMarginsFont(font);
    QsciEdit->setPaper(QColor(19, 67, 79));
    QFontMetrics fontmetrics = QFontMetrics(font);
    //设置左侧行号栏宽度等
    QsciEdit->setMarginWidth(0, fontmetrics.width("000"));
    QsciEdit->setMarginLineNumbers(0, true);
    QsciEdit->setBraceMatching(QsciScintilla::SloppyBraceMatch);
    QsciEdit->setTabWidth(4);
    //设置括号等自动补全
    QsciEdit->setAutoIndent(true);
    //初始设置c++解析器
    //textEdit->setLexer(new QsciLexerCPP(this));
    QscilexerCppAttach *textLexer = new QscilexerCppAttach;
    textLexer->setColor(QColor(Qt:: green),QsciLexerCPP::CommentLine);    //设置自带的注释行为绿色
    textLexer->setColor(QColor(Qt::red),QsciLexerCPP::KeywordSet2);
    QsciEdit->setLexer(textLexer);
    //设置自动补全

    QsciAPIs *apis = new QsciAPIs(textLexer);
    apis->add(QString("PORT_DEF_BEGIN"));
    apis->add(QString("PORT_DEF_END"));
    apis->add(QString("DEF"));
    apis->add(QString("PORT"));
    apis->add(QString("FUNCTION"));
    apis->add(QString("METER"));
    apis->add(QString("METER_PIC"));
    apis->add(QString("METER_MODE"));

    apis->prepare();

    QFont font1("Courier", 10, QFont::Normal);
    //    this->setFont(font1);

    QsciEdit->setAutoCompletionSource(QsciScintilla::AcsAll);   //设置源，自动补全所有地方出现的
    QsciEdit->setAutoCompletionCaseSensitivity(true);   //设置自动补全大小写敏感
    QsciEdit->setAutoCompletionThreshold(2);    //设置每输入2个字符就会出现自动补全的提示

    QsciEdit->setCaretLineVisible(true);
    //设置光标所在行背景色
    QsciEdit->setCaretLineBackgroundColor(Qt::lightGray);

    // ui->textEdit->setCursorPosition(2,2);
    //int markerDefine(MarkerSymbol sym, int markerNumber = -1);
    QsciEdit->SendScintilla(QsciScintilla::SCI_SETCODEPAGE,QsciScintilla::SC_CP_UTF8);//设置编码为UTF-8
    //得到光标位置
    int line,col;
    QsciEdit->getCursorPosition(&line,&col);

    //设置显示字体
    QsciEdit->setFont(QFont("Courier New"));
    //设置编码方式
    QsciEdit->SendScintilla(QsciScintilla::SCI_SETCODEPAGE,QsciScintilla::SC_CP_UTF8);//设置编码为UTF-8
}

//载入元件对应的诊断参数
void DialogUnitManage::LoadDiagnoseParameters(int Equipment_ID)
{
    ui->tableUnitDiagnosePara->setRowCount(0);
    QSqlQuery QueryEquipmentDiagnosePara = QSqlQuery(T_LibDatabase);//设置数据库选择模型
    QString SqlStr;
    SqlStr = "SELECT * FROM EquipmentDiagnosePara WHERE Equipment_ID = '"+QString::number(Equipment_ID)+"'";
    QueryEquipmentDiagnosePara.exec(SqlStr);
    while(QueryEquipmentDiagnosePara.next())
    {
        ui->tableUnitDiagnosePara->setRowCount(ui->tableUnitDiagnosePara->rowCount()+1);
        ui->tableUnitDiagnosePara->setItem(ui->tableUnitDiagnosePara->rowCount()-1,0,new QTableWidgetItem(QString::number(ui->tableUnitDiagnosePara->rowCount())));//序号
        ui->tableUnitDiagnosePara->item(ui->tableUnitDiagnosePara->rowCount()-1,0)->setFlags(ui->tableUnitDiagnosePara->item(ui->tableUnitDiagnosePara->rowCount()-1,0)->flags()&(~Qt::ItemIsEditable));
        ui->tableUnitDiagnosePara->item(ui->tableUnitDiagnosePara->rowCount()-1,0)->setData(Qt::UserRole,QueryEquipmentDiagnosePara.value("DiagnoseParaID").toString());
        ui->tableUnitDiagnosePara->setItem(ui->tableUnitDiagnosePara->rowCount()-1,1,new QTableWidgetItem(QueryEquipmentDiagnosePara.value("Name").toString()));//名称
        ui->tableUnitDiagnosePara->setItem(ui->tableUnitDiagnosePara->rowCount()-1,2,new QTableWidgetItem(QueryEquipmentDiagnosePara.value("Unit").toString()));//单位
        ui->tableUnitDiagnosePara->setItem(ui->tableUnitDiagnosePara->rowCount()-1,3,new QTableWidgetItem(QueryEquipmentDiagnosePara.value("DefaultValue").toString()));//默认值
        ui->tableUnitDiagnosePara->setItem(ui->tableUnitDiagnosePara->rowCount()-1,4,new QTableWidgetItem(QueryEquipmentDiagnosePara.value("Remark").toString()));//备注
    }
}

//添加诊断参数
void DialogUnitManage::on_BtnAddPara_clicked()
{
    ui->tableUnitDiagnosePara->setRowCount(ui->tableUnitDiagnosePara->rowCount()+1);
    ui->tableUnitDiagnosePara->setItem(ui->tableUnitDiagnosePara->rowCount()-1,0,new QTableWidgetItem(QString::number(ui->tableUnitDiagnosePara->rowCount())));//序号
    ui->tableUnitDiagnosePara->item(ui->tableUnitDiagnosePara->rowCount()-1,0)->setFlags(ui->tableUnitDiagnosePara->item(ui->tableUnitDiagnosePara->rowCount()-1,0)->flags()&(~Qt::ItemIsEditable));
    ui->tableUnitDiagnosePara->setItem(ui->tableUnitDiagnosePara->rowCount()-1,1,new QTableWidgetItem(""));//名称
    ui->tableUnitDiagnosePara->setItem(ui->tableUnitDiagnosePara->rowCount()-1,2,new QTableWidgetItem(""));//单位
    ui->tableUnitDiagnosePara->setItem(ui->tableUnitDiagnosePara->rowCount()-1,3,new QTableWidgetItem("0"));//默认值
    ui->tableUnitDiagnosePara->setItem(ui->tableUnitDiagnosePara->rowCount()-1,4,new QTableWidgetItem(""));//备注
}

void DialogUnitManage::on_BtnDeletePara_clicked()
{
    if(ui->tableUnitDiagnosePara->currentRow()<0) return;
    ui->tableUnitDiagnosePara->removeRow(ui->tableUnitDiagnosePara->currentRow());
}

void DialogUnitManage::on_CbAllSourceConn_clicked()
{
    if(ui->CbAllSourceConn->isChecked())
    {
        for(int i=0;i<ui->tableWidgetSpur->rowCount();i++)
        {
            ui->tableWidgetSpur->item(i,2)->setCheckState(Qt::Checked);
        }
    }
    else
    {
        for(int i=0;i<ui->tableWidgetSpur->rowCount();i++)
        {
            ui->tableWidgetSpur->item(i,2)->setCheckState(Qt::Unchecked);
        }
    }
}

//编译器件描述文本
void DialogUnitManage::on_BtnCompile_clicked()
{
    ui->tableWidgetStructure->setRowCount(0);
    //提取Enum
    QString StrUnitDesc=QsciEdit->text();
    QStringList ListEnumName,ListEnumTypeName,ListEnumVal,ListIniVal,ListCmdObsVal;
    CompileStructure(StrUnitDesc,"",ListEnumName,ListEnumTypeName,ListEnumVal,ListIniVal,ListCmdObsVal);
    //添加子器件的enum
    QStringList SubComponentList=GetSubComponentList(QsciEdit->text());
    for(QString StrSubComponent:SubComponentList)
    {
        QSqlQuery QueryFunctionDefineClass(T_LibDatabase);
        QString StrSql="SELECT * FROM FunctionDefineClass WHERE TClassName = '"+StrSubComponent.split(",").at(0)+"'";
        QueryFunctionDefineClass.exec(StrSql);
        if(QueryFunctionDefineClass.next())
        {
            QString SubModuleTModel=QueryFunctionDefineClass.value("TModel").toString();
            CompileStructure(SubModuleTModel,StrSubComponent.split(",").at(1),ListEnumName,ListEnumTypeName,ListEnumVal,ListIniVal,ListCmdObsVal);
        }
    }

    for(int i=0;i<ListEnumName.count();i++)
    {
        ui->tableWidgetStructure->setRowCount(ui->tableWidgetStructure->rowCount()+1);
        ui->tableWidgetStructure->setItem(ui->tableWidgetStructure->rowCount()-1,0,new QTableWidgetItem(ListEnumName.at(i)));
        ui->tableWidgetStructure->setItem(ui->tableWidgetStructure->rowCount()-1,1,new QTableWidgetItem(ListEnumTypeName.at(i)));
        QComboBox *CbInitVal=new QComboBox();
        CbInitVal->addItems(ListEnumVal.at(i).split(","));
        CbInitVal->setCurrentText(ListIniVal.at(i));
        ui->tableWidgetStructure->setCellWidget(ui->tableWidgetStructure->rowCount()-1,2,CbInitVal);
        QComboBox *CbCommandOrObservable=new QComboBox();
        CbCommandOrObservable->addItems({"Commandable","Observable","undefined","default"});
        CbCommandOrObservable->setCurrentText(ListCmdObsVal.at(i));
        ui->tableWidgetStructure->setCellWidget(ui->tableWidgetStructure->rowCount()-1,3,CbCommandOrObservable);
    }
}


void DialogUnitManage::on_tableRepairInfo_clicked(const QModelIndex &index)
{
    if(!index.isValid()) return;
    ui->TextEdRepairPlan->setText(ui->tableRepairInfo->item(index.row(),2)->text());
    ui->TextEdRepairResource->setText(ui->tableRepairInfo->item(index.row(),3)->text());
}

void DialogUnitManage::on_TextEdRepairPlan_textChanged()
{
    if(ui->tableRepairInfo->currentRow()<0) return;
    ui->tableRepairInfo->item(ui->tableRepairInfo->currentRow(),2)->setText(ui->TextEdRepairPlan->toPlainText());
}

void DialogUnitManage::on_TextEdRepairResource_textChanged()
{
    if(ui->tableRepairInfo->currentRow()<0) return;
    ui->tableRepairInfo->item(ui->tableRepairInfo->currentRow(),3)->setText(ui->TextEdRepairResource->toPlainText());
}

void DialogUnitManage::on_BtnHideShow_clicked()
{
    if(ui->BtnHideShow->text()=="<")
    {
        //ui->widget_2->setVisible(false);
        ui->widget_Search->setVisible(false);
        ui->tableWidgetUnit->setVisible(false);
        ui->groupBox->setVisible(false);
        ui->BtnHideShow->setText(">");
    }
    else
    {
        //ui->widget_2->setVisible(true);
        ui->widget_Search->setVisible(true);
        ui->tableWidgetUnit->setVisible(true);
        ui->groupBox->setVisible(true);
        ui->BtnHideShow->setText("<");
    }
}

void DialogUnitManage::on_BtnInsertSpur_clicked()
{
    if(ui->tableWidgetSpur->currentRow()<0) return;
    //打开功能选择窗口
    dlgFuncDefine->Canceled=true;
    dlgFuncDefine->setModal(true);
    dlgFuncDefine->exec();
    if(dlgFuncDefine->Canceled) return;
    //更新table中的功能定义和数据库
    ui->tableWidgetSpur->insertRow(ui->tableWidgetSpur->currentRow());
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->currentRow()-1,0,new QTableWidgetItem(dlgFuncDefine->FunctionDefineName));
    ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow()-1,0)->setFlags(ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow()-1,0)->flags()&(~Qt::ItemIsEditable));
    ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow()-1,0)->setData(Qt::UserRole,QVariant(dlgFuncDefine->FunctionDefineClass_ID));
    //根据选择的功能是几个Term决定
    QString ConnNum;
    QSqlQuery QueryVar = QSqlQuery(T_LibDatabase);//设置数据库选择模型
    QString temp = "SELECT * FROM FunctionDefineClass WHERE FunctionDefineClass_ID = '"+dlgFuncDefine->FunctionDefineClass_ID+"'";//SPSFuncDef WHERE FuncDef = '"+dlgFuncDefine->FunctionDefineClass_ID+"'";
    QueryVar.exec(temp);
    if(QueryVar.next())//
    {
        for(int i=0;i<GetDwgTermCount("C:/TBD/SPS/"+QueryVar.value("DefaultSymbol").toString()+".dwg",QueryVar.value("DefaultSymbol").toString());i++)
        {
            if(i>0) ConnNum+="￤";
            ConnNum+=QString::number(i+1);
        }
    }
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->currentRow()-1,1,new QTableWidgetItem(ConnNum));
    qDebug()<<"dlgFuncDefine->FuncType="<<dlgFuncDefine->FuncType;
    QTableWidgetItem *ItemSourceConn = new QTableWidgetItem("");
    if(dlgFuncDefine->FuncType=="接线端口") ItemSourceConn->setCheckState(Qt::Unchecked);
    //ItemSourceConn->setFlags(ItemSourceConn->flags()&(~Qt::ItemIsEditable));
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->currentRow()-1,2,ItemSourceConn);

    QTableWidgetItem *ItemExecConn = new QTableWidgetItem("");
    if(dlgFuncDefine->FuncType=="接线端口") ItemExecConn->setCheckState(Qt::Unchecked);
    ItemExecConn->setFlags(ItemExecConn->flags()&(~Qt::ItemIsEditable));
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->currentRow()-1,3,ItemExecConn);

    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->currentRow()-1,4,new QTableWidgetItem(""));
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->currentRow()-1,5,new QTableWidgetItem("")); //符号
    ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow()-1,5)->setFlags(ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow(),5)->flags()&(~Qt::ItemIsEditable));
    ui->tableWidgetSpur->item(ui->tableWidgetSpur->currentRow()-1,5)->setData(Qt::UserRole,QVariant(0));
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->currentRow()-1,6,new QTableWidgetItem(""));
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->currentRow()-1,7,new QTableWidgetItem(dlgFuncDefine->FuncType));//功能类型
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->currentRow()-1,8,new QTableWidgetItem(""));//测试代价
    ui->tableWidgetSpur->setItem(ui->tableWidgetSpur->currentRow()-1,9,new QTableWidgetItem(""));//受控
}

//Lu ToDo 显示器件图片
void DialogUnitManage::on_tableWidgetUnitPic_clicked(const QModelIndex &index)
{
    //LoadLbPicture(ui->LbUnitPic1,ui->tableWidgetUnitPic->item(index.row(),0)->data(Qt::UserRole).toString());
    QString picPath = ui->tableWidgetUnitPic->item(index.row(), 0)->data(Qt::UserRole).toString();
    QPixmap pix(picPath);
    if(pix.isNull()) {
        qDebug() << "Failed to load the image:"<< picPath;
        return;
    }
    m_scene_unit.SetBackGroundImage(pix);
    ui->graphicsView_Unit->ScaleToWidget();

}

void DialogUnitManage::on_BtnFromUnitImage_clicked()
{
    if(CurUnitImageIndex<ui->tableWidgetUnitPic->rowCount())
    {
        //LoadLbPicture(Label_Image,ui->tableWidgetUnitPic->item(CurUnitImageIndex,0)->data(Qt::UserRole).toString());
        QPixmap pix(ui->tableWidgetUnitPic->item(CurUnitImageIndex,0)->data(Qt::UserRole).toString());
        m_scene.clear();
        m_scene.setSceneRect(0, 0, pix.width(), pix.height());
        m_scene.addPixmap(pix)->setPos(0, 0);
        //m_scene.SetBackGroundImage(pix, ui->graphicsView->width(), ui->graphicsView->height());
        CurImgPath=ui->tableWidgetUnitPic->item(CurUnitImageIndex,0)->data(Qt::UserRole).toString();
    }
    CurUnitImageIndex++;
    if(CurUnitImageIndex>=ui->tableWidgetUnitPic->rowCount()) CurUnitImageIndex=0;
}

void DialogUnitManage::on_BtnFromDisk_clicked()
{
    QFileDialog fileDialog(this);
    fileDialog.setWindowTitle(tr("选择文件"));
    fileDialog.setDirectory(PIC_BASE_PATH);
    fileDialog.setNameFilter(tr("Images (*.png *.xpm *.jpg *.jpeg *.gif *.webp)"));
    // fileDialog->setFileMode(QFileDialog::ExistingFiles);
    fileDialog.setViewMode(QFileDialog::Detail);
    if (!fileDialog.exec()) return;
    QStringList fileNames=fileDialog.selectedFiles();
    if(fileNames.count()!=1)
    {
        QMessageBox::warning(nullptr, "提示", "请选择一张图片！");
        return;
    }
    QPixmap pix(fileNames.at(0));
    //m_scene.SetBackGroundImage(pix, ui->graphicsView->width(), ui->graphicsView->height());
    m_scene.clear();
    m_scene.setSceneRect(0, 0, pix.width(), pix.height());
    m_scene.addPixmap(pix)->setPos(0, 0);
    CurImgPath=fileNames.at(0);
}

void DialogUnitManage::on_BtnSign_clicked()
{

}

void DialogUnitManage::SlotChangeColorWrapper(QColor mColor)
{
    SlotChangeColor(ui->graphicsView,mColor);
}

void DialogUnitManage::SlotDrawTagWrapper(int Type,QColor mColor)
{
    SlotDrawTag(ui->graphicsView,Type,mColor);
}

void DialogUnitManage::on_BtnDelItem_clicked()
{
    QList<QTableWidgetItem*> selectedItems = ui->tableTerm->selectedItems();
    QSet<int> rowsToDeleteSet;

    // 遍历选中的项，收集要删除的行号
    foreach(QTableWidgetItem* item, selectedItems) {
        rowsToDeleteSet.insert(item->row());
    }

    // 转换为列表并排序，以便从最大行号开始删除
    QList<int> rowsToDelete = rowsToDeleteSet.toList();
    std::sort(rowsToDelete.begin(), rowsToDelete.end(), std::greater<int>());

    QSqlQuery query(T_LibDatabase);
    foreach(int row, rowsToDelete) {
        QString termID = ui->tableTerm->item(row, 1)->data(Qt::UserRole).toString();
        if(!termID.isEmpty()) {
            // 从TermInfo中删除对应的记录
            QString deleteSql = QString("DELETE FROM TermInfo WHERE Term_ID = '%1'").arg(termID);
            if(!query.exec(deleteSql)) qDebug() << "Error deleting TermInfo record:" << query.lastError().text();
            else qDebug() << "Deleted TermInfo record with Term_ID:" << termID;
        }
        // 从tableTerm中删除行
        ui->tableTerm->removeRow(row);
    }
}

/*
图片及标注信息存储格式：
ImageFileName*TagType/TagColor/TagPos/TagEdge，如果有多个图片，用||分隔。标注信息是可选的，不是必须的。图片名称ImageFileName只包含文件名，不包含路径。
MURR.85.jpg*5/0/97.00,120.00/56.00,111.00||MURR01.jpg||MURR02.jpg*5/1/27.00,110.00/12.00,40.00
*/

//子块代号， ui->tableTerm->item(,0):EquipmentTemplate_ID
//端号， ui->tableTerm->item(,1):Term_ID
void DialogUnitManage::on_BtnSave_clicked()
{
    if(ui->tableTerm->currentRow() < 0) {
        QMessageBox::warning(nullptr, "提示", "请选择有效端口后重试！");
        return;
    }

    QSqlQuery queryTermInfo(T_LibDatabase);
    QString termID = ui->tableTerm->item(ui->tableTerm->currentRow(), 1)->data(Qt::UserRole).toString();
    bool isExistingTerm = !termID.isEmpty();

    QString sqlStr;
    if(isExistingTerm) {
        sqlStr = "UPDATE TermInfo SET TermNum = :TermNum, TermDesc = :TermDesc, TestCost = :TestCost, TermPicPath = :TermPicPath WHERE Term_ID = :Term_ID";
        queryTermInfo.prepare(sqlStr);
        queryTermInfo.bindValue(":Term_ID", termID);
    } else {
        termID = QString::number(GetMaxIDOfLibDatabase(T_LibDatabase, "TermInfo", "Term_ID"));
        sqlStr = "INSERT INTO TermInfo (Term_ID, Equipment_ID, EquipmentTemplate_ID, TermNum, TermDesc, TestCost, TermPicPath)"
                 "VALUES (:Term_ID, :Equipment_ID, :EquipmentTemplate_ID, :TermNum, :TermDesc, :TestCost, :TermPicPath)";
        queryTermInfo.prepare(sqlStr);
        queryTermInfo.bindValue(":Term_ID", termID);
        queryTermInfo.bindValue(":Equipment_ID", CurEquipment_ID);
        queryTermInfo.bindValue(":EquipmentTemplate_ID", ui->tableTerm->item(ui->tableTerm->currentRow(), 0)->data(Qt::UserRole).toString());
    }
    queryTermInfo.bindValue(":TermNum", ui->tableTerm->item(ui->tableTerm->currentRow(), 1)->text());
    queryTermInfo.bindValue(":TermDesc", ui->tableTerm->item(ui->tableTerm->currentRow(), 2)->text());
    queryTermInfo.bindValue(":TestCost", ui->tableTerm->item(ui->tableTerm->currentRow(), 3)->text());

    // 获取CurImgPath中的目录路径和文件名
    QFileInfo fileInfo(CurImgPath);
    QString fileName = fileInfo.fileName();

    // 使用辅助函数来复制图片并接收新的图片路径
    QString baseDirPath = QString(PIC_BASE_PATH) + "/" + CurEquipment_Supplier;
    CurImgPath = copyImageToDirectory(CurImgPath, baseDirPath, "tag");

    QString StrTagInfo=genTagInfoFromScene(m_scene,(int)m_dialogTag->CurTagColor);

    // 构建 "文件名*标签信息" 格式的字符串
    QString termPicPath = fileName;
    if (isTagInfoValid(StrTagInfo)) {
        termPicPath += "*" + StrTagInfo;
    }
    else StrTagInfo = "";

    queryTermInfo.bindValue(":TermPicPath", termPicPath); // 绑定处理后的路径

    if(!queryTermInfo.exec()) {
        qDebug() << "Error executing SQL query:" << queryTermInfo.lastError().text();
    }
    if(!isExistingTerm)ui->tableTerm->item(ui->tableTerm->currentRow(), 1)->setData(Qt::UserRole, termID);
    ui->tableTerm->item(ui->tableTerm->currentRow(), 5)->setText(CurImgPath);
    ui->tableTerm->item(ui->tableTerm->currentRow(), 4)->setText((StrTagInfo.isEmpty() || CurImgPath.isEmpty()) ? "否" : "是");
    ui->tableTerm->item(ui->tableTerm->currentRow(), 4)->setData(Qt::UserRole, StrTagInfo);
}

void DialogUnitManage::on_tableTerm_clicked(const QModelIndex &index)
{
    m_scene.clear();
    QPixmap pix(ui->tableTerm->item(index.row(),5)->text());

    m_scene.SetBackGroundImage(pix);
    ui->graphicsView->ScaleToWidget();

    CurImgPath=ui->tableTerm->item(index.row(),5)->text();
    CurUnitImageIndex=0;
    //qDebug()<<ui->tableTerm->item(ui->tableTerm->currentRow(),4)->data(Qt::UserRole).toString();
    LoadPicTag(ui->tableTerm->item(ui->tableTerm->currentRow(),4)->data(Qt::UserRole).toString(),ui->graphicsView);
}

void DialogUnitManage::on_BtnCancelSign_clicked()
{
    //m_scene.items().clear();
    QList<QGraphicsItem *> list = m_scene.items();
    for(int i=0;i<list.count();i++)
    {
        if(list[i]->type()!=7)
        {
            m_scene.removeItem(list[i]);
        }
    }
}

//void DialogUnitManage::on_BtnUpdatePath_clicked()
//{
//    QSqlQuery QueryVar = QSqlQuery(T_LibDatabase);//设置数据库选择模型
//    QString temp = "SELECT Equipment_ID,Picture FROM Equipment WHERE Equipment_ID = '"+CurEquipment_ID+"'";
//    qDebug()<<"on_BtnUpdatePath_clicked:"<<temp;
//    qDebug()<<QueryVar.exec(temp);
//    QString NewStr;
//    while(QueryVar.next())
//    {
//       QStringList ListPicPath=QueryVar.value(1).toString().split("|");
//       qDebug()<<"ListPicPath"<<ListPicPath;
//       for(QString CurPicPath:ListPicPath)
//       {
//           qDebug()<<"CurPicPath"<<CurPicPath;
//           QFileInfo FilePic(CurPicPath);
//           if(!FilePic.exists())
//           {
//               QString FindPic=FindLocalFileFromPath("C:/TBD/data/UnitImage", CurPicPath);
//               qDebug()<<"FindPic:"<<FindPic;
//               if(FindPic!="")
//               {
//                  if(NewStr!="") NewStr+="|";
//                  NewStr+=FindPic;
//               }
//           }
//           else
//           {
//               if(NewStr!="") NewStr+="|";
//               NewStr+=CurPicPath;
//           }
//       }
//       QSqlQuery QueryUpdate = QSqlQuery(T_LibDatabase);//设置数据库选择模型
//       QString SqlStr="UPDATE Equipment SET Picture=:Picture WHERE Equipment_ID = '"+CurEquipment_ID+"'";
//       qDebug()<<"SqlStr:"<<SqlStr;
//       qDebug()<<QueryUpdate.prepare(SqlStr);
//       qDebug()<<"NewStr:"<<NewStr;
//       QueryUpdate.bindValue(":Picture",NewStr);
//       if (!QueryUpdate.exec()) qDebug() << "Update query failed:" << QueryUpdate.lastError().text();
//       else qDebug() << "Update query executed successfully.";
//    }
//}
void DialogUnitManage::on_BtnUpdatePath_clicked()
{
    QSqlQuery QueryVar(T_LibDatabase); // 设置数据库选择模型
    QString selectQuery = "SELECT Equipment_ID, Picture FROM Equipment";

    if (!QueryVar.exec(selectQuery)) {
        qDebug() << "Select query failed:" << QueryVar.lastError().text();
        return;
    }

    while (QueryVar.next()) {
        QString CurEquipment_ID = QueryVar.value(0).toString();
        QStringList ListPicPath = QueryVar.value(1).toString().split("||");
        QString NewStr;

        for (const QString& CurPicPath : ListPicPath) {
            QFileInfo FilePic(CurPicPath);
            if (!FilePic.exists()) {
                QString FindPic = FindLocalFileFromPath(PIC_BASE_PATH, CurPicPath);
                if (!FindPic.isEmpty()) {
                    if (!NewStr.isEmpty()) NewStr += "||";
                    NewStr += FindPic;
                }
            } else {
                if (!NewStr.isEmpty()) NewStr += "||";
                NewStr += CurPicPath;
            }
        }

        QSqlQuery QueryUpdate(T_LibDatabase); // 设置数据库选择模型
        QString SqlStr = "UPDATE Equipment SET Picture=:Picture WHERE Equipment_ID = '" + CurEquipment_ID + "'";

        if (!QueryUpdate.prepare(SqlStr)) {
            qDebug() << "Prepare query failed for Equipment_ID" << CurEquipment_ID << ":" << QueryUpdate.lastError().text();
            continue;
        }

        QueryUpdate.bindValue(":Picture", NewStr);

        if (!QueryUpdate.exec()) {
            qDebug() << "Update query failed for Equipment_ID" << CurEquipment_ID << ":" << QueryUpdate.lastError().text();
        }
    }
}


void DialogUnitManage::on_BtnCategory_clicked()
{
    QDialog dialog(this);
    QVBoxLayout layout(&dialog);
    QComboBox *cbUnitGroup = new QComboBox(&dialog);
    QComboBox *cbUnitSubGroup = new QComboBox(&dialog);
    QPushButton btnOk("确定", &dialog), btnCancel("取消", &dialog);

    // 使用值捕获
    connect(cbUnitGroup, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [=](int index) {
        cbUnitSubGroup->clear();
        QString selectedParentId = cbUnitGroup->currentData().toString();

        QSqlQuery subQuery(T_LibDatabase);
        QString subSql = "SELECT * FROM Class WHERE Level = '2' AND ParentNo = '" + selectedParentId + "'";
        subQuery.exec(subSql);
        while (subQuery.next()) {
            QString subClassId = subQuery.value("Class_ID").toString();
            cbUnitSubGroup->addItem(subQuery.value("Desc").toString(), subClassId);
        }
    });

    // 加载类别和子类别信息
    QSqlQuery query(T_LibDatabase);
    QString sqlStr = "SELECT * FROM Class WHERE Level = '1'";
    query.exec(sqlStr);
    while (query.next()) {
        QString classId = query.value("Class_ID").toString();
        QString desc = query.value("Desc").toString();
        cbUnitGroup->addItem(desc, classId);
    }

    // 默认触发一次更新，以填充初始子类别
    if (cbUnitGroup->count() > 0) {
        emit cbUnitGroup->currentIndexChanged(cbUnitGroup->currentIndex());
    }

    layout.addWidget(cbUnitGroup);
    layout.addWidget(cbUnitSubGroup);
    QHBoxLayout buttonLayout;
    buttonLayout.addWidget(&btnOk);
    buttonLayout.addWidget(&btnCancel);
    layout.addLayout(&buttonLayout);

    connect(&btnOk, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(&btnCancel, &QPushButton::clicked, &dialog, &QDialog::reject);

    int selectedRows = 0, processedRows = 0;
    if (dialog.exec() == QDialog::Accepted) {
        QString selectedClassId = cbUnitSubGroup->currentData().toString();

        QList<QTableWidgetSelectionRange> selectedRanges = ui->tableWidgetUnit->selectedRanges();
        foreach (const QTableWidgetSelectionRange &range, selectedRanges) {
            for (int row = range.topRow(); row <= range.bottomRow(); ++row) {
                // 跳过被隐藏的行
                if(ui->tableWidgetUnit->isRowHidden(row))continue;

                QTableWidgetItem *item = ui->tableWidgetUnit->item(row, 0);
                if (item) {
                    selectedRows++;
                    QString equipmentId = item->data(Qt::UserRole).toString();
                    QSqlQuery updateQuery(T_LibDatabase);
                    QString updateSql = "UPDATE Equipment SET Class_ID = '" + selectedClassId + "' WHERE Equipment_ID = '" + equipmentId + "'";
                    if (updateQuery.exec(updateSql)) {
                        processedRows++;
                    } else {
                        qDebug() << "Failed to process row" << row << "with Equipment_ID:" << equipmentId;
                    }
                }
            }
        }
        QMessageBox::information(&dialog, "处理结果", QString("选中了 %1 行，成功处理了 %2 行").arg(selectedRows).arg(processedRows));
    }
}
void DialogUnitManage::on_BtnCountUnit_clicked()
{
    // 获取所有相关类别的器件数量
    QMap<QString, int> classItemCountMap;
    QSqlQuery query(T_LibDatabase);
    query.exec("SELECT Class_ID, COUNT(*) FROM Equipment GROUP BY Class_ID");
    while (query.next()) {
        classItemCountMap.insert(query.value(0).toString(), query.value(1).toInt());
    }

    //    // 获取Class_ID对应的描述
    //    QMap<QString, QString> classIdDescMap;
    //    QSqlQuery classQuery(T_LibDatabase);
    //    classQuery.exec("SELECT Class_ID, Desc FROM Class");
    //    while (classQuery.next()) {
    //        classIdDescMap.insert(classQuery.value(0).toString(), classQuery.value(1).toString());
    //    }

    //    // 输出classItemCountMap的内容及对应的描述
    //    QMapIterator<QString, int> i(classItemCountMap);
    //    while (i.hasNext()) {
    //        i.next();
    //        QString classId = i.key();
    //        int itemCount = i.value();
    //        QString desc = classIdDescMap.value(classId, "未知类别");

    //        qDebug() << "Class_ID:" << classId << ", Desc:" << desc << ", Count:" << itemCount;

    //        // 如果是未知类别，打印该类别的Equipment_ID
    //        if(desc == "未知类别") {
    //            QSqlQuery unknownQuery(T_LibDatabase);
    //            unknownQuery.exec(QString("SELECT Equipment_ID FROM Equipment WHERE Class_ID='%1'").arg(classId));
    //            while (unknownQuery.next()) {
    //                QString equipmentId = unknownQuery.value(0).toString();
    //                qDebug() << "未知类别的 Equipment_ID:" << equipmentId;
    //            }
    //        }
    //    }

    // 更新器件数量
    for (int i = 0; i < Model->rowCount(); ++i) {
        QStandardItem *firstLevelItem = Model->item(i);
        int totalItemCountLevel1 = 0;

        for (int j = 0; j < firstLevelItem->rowCount(); ++j) {
            QStandardItem *secondLevelItem = firstLevelItem->child(j);
            int totalItemCountLevel2 = 0;

            for (int k = 0; k < secondLevelItem->rowCount(); ++k) {
                QStandardItem *thirdLevelItem = secondLevelItem->child(k);
                QString classIdLevel3 = thirdLevelItem->data(Qt::UserRole).toString();

                int itemCountLevel3 = classItemCountMap.value(classIdLevel3, 0);

                // 清除原有的统计数据
                QString originalText = thirdLevelItem->text().split(" (").first();
                thirdLevelItem->setText(QString("%1 (%2)").arg(originalText).arg(itemCountLevel3));

                totalItemCountLevel2 += itemCountLevel3;
            }

            QString originalText = secondLevelItem->text().split(" (").first();
            secondLevelItem->setText(QString("%1 (%2)").arg(originalText).arg(totalItemCountLevel2));
            totalItemCountLevel1 += totalItemCountLevel2;
        }

        QString originalText = firstLevelItem->text().split(" (").first();
        firstLevelItem->setText(QString("%1 (%2)").arg(originalText).arg(totalItemCountLevel1));
    }

    // 弹出对话框显示未统计的记录;

}

void DialogUnitManage::on_BtnCopyUnitPic_clicked()
{
    QString UnitJpgPath;
    for(int i = 0; i < ui->tableWidgetUnitPic->rowCount(); i++) {
        if(!UnitJpgPath.isEmpty()) UnitJpgPath += "|";
        UnitJpgPath += ui->tableWidgetUnitPic->item(i, 0)->data(Qt::UserRole).toString();
    }

    if(UnitJpgPath.isEmpty()) return;

    int selectedRows = 0, processedRows = 0;
    QList<QTableWidgetSelectionRange> selectedRanges = ui->tableWidgetUnit->selectedRanges();

    foreach (const QTableWidgetSelectionRange &range, selectedRanges) {
        for (int row = range.topRow(); row <= range.bottomRow(); ++row) {
            // 跳过被隐藏的行
            if(ui->tableWidgetUnit->isRowHidden(row))continue;

            QTableWidgetItem *item = ui->tableWidgetUnit->item(row, 0);
            if (item) {
                selectedRows++;
                QString equipmentId = item->data(Qt::UserRole).toString();
                QSqlQuery query(T_LibDatabase);

                // 检查原Picture字段是否为空
                QString checkSql = "SELECT Picture FROM Equipment WHERE Equipment_ID = '" + equipmentId + "'";
                if (query.exec(checkSql) && query.next() && query.value(0).toString().isEmpty()) {
                    QString updateSql = "UPDATE Equipment SET Picture = '" + UnitJpgPath + "' WHERE Equipment_ID = '" + equipmentId + "'";
                    if (query.exec(updateSql)) {
                        processedRows++;
                    } else {
                        qDebug() << "Failed to process row" << row << "with Equipment_ID:" << equipmentId << " Error:" << query.lastError().text();
                    }
                }
            }
        }
    }

    QMessageBox::information(this, "处理结果", QString("选中了 %1 行，成功处理了 %2 行").arg(selectedRows).arg(processedRows));
}

//void DialogUnitManage::on_BtnCheck_clicked()
//{
//    // 获取代码编辑器中的代码
//    QString code = QsciEdit->text();

//    // 初始化检查结果
//    QVector<CodeError> errors;

//    // 假设已实现检查逻辑，将错误添加到errors向量中...
//    // 示例错误：
//    errors.append({3, "DEF ELECTRIC_PORT \"(10) 21\"", "无效的端口定义"});

//    // 生成校核结果文本
//    QString checkResult;
//    for (const auto &error : errors)
//    {
//        checkResult += "<p>Line " + QString::number(error.lineNumber) + ": <span style='color:red'>" + error.codeSegment + "</span></p>";
//        checkResult += "<p>错误原因: " + error.reason + "</p>";
//    }

//    // 显示CodeCheckDialog
//    CodeCheckDialog dialog(this);
//    dialog.setCheckResult(checkResult,1);
//    dialog.exec();
//}
void DialogUnitManage::on_BtnCheck_clicked()
{
    QString code = QsciEdit->text();
    CodeChecker checker(code);  // 创建CodeChecker对象
    QVector<CodeError> errors = checker.check();  // 进行代码检查
    //errors.append({4, "DEF ELECTRIC_PORT \"3\" \"20211212125501\" AS \"EP1\"", "无效的端口定义"});
    //errors.append({11, "PORT_DEFEND", "语法错误"});
    //errors.append({20, "	il=-i2 ^ i2 > 0 ^ i2=(u1-u2)/r1;}", "变量未定义"});

    CodeCheckDialog dialog(this);
    dialog.setCheckResult(errors);
    dialog.exec();
}
