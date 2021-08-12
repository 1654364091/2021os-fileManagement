#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <QtWidgets/QMainWindow>

#include <stdio.h>
#include <memory.h>
#include <string>
#include <iostream>
#include <QMessageBox>
#include <QInputDialog>
#include <vector>
#include <QtDebug>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QCloseEvent>
using namespace std;
#define GENERAL 1		//1代表普通文件2代表目录文件0表示空文件
#define DIRECTORY 2
#define Zero 0


QT_BEGIN_NAMESPACE
namespace Ui { class fileSystem; }
QT_END_NAMESPACE

class fileSystem : public QMainWindow
{
    Q_OBJECT

public:
    void drawDir(int i, const char* dirName);		//绘制文件夹目录
    void drawFile(int i, const char* fileName);	//绘制文件目录
    int  format();							//格式化
    int  mkdir(const char* sonfname);				//新建文件夹
    int rmdir(const char* sonfname);				//删除文件夹
    int create(const char* name);					//新建文件
    int listshow();							//更新目录
    int delfile(const char* name);				//删除文件
    int changePath(const char* sonfname);			//切换路径
    int write(const char* name, const char* content);	//修改文件
    int save();								//保存
    int open(const char* file);					//打开文件
    int close(const char* file);					//关闭文件1
    int close2(const char* file, const char* fileContent);	//关闭文件 传递文件内容
    bool canClose(const char* file);				//可否关闭文件
    int  read(const char* file, string* content, int* size);	//查看文件

    void messageBox(const char* s);				//消息对话框
    void updateLayout();					//更新布局
    void Initial(int i);							//热重载

    fileSystem(QWidget* parent = nullptr);
    ~fileSystem();
    QVBoxLayout* dirLayout;					//文件夹 文件 文件大小 路径布局
    QVBoxLayout* fileLayout;
    QVBoxLayout* sizeLayout;
    QHBoxLayout* pathLayout;

public slots:			//槽函数
    void addDirFuc();	//新建文件夹
    void addFile();		//新建文件
    void backDir();		//返回上级目录
    void formatAll();	//格式化磁盘
    void saveAll();		//保存并退出


private:
    Ui::fileSystem *ui;
};
#endif // FILESYSTEM_H
