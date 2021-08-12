#include "filesystem.h"
#include "ui_filesystem.h"


struct FCB {
    char fname[16];			//文件名
    char type;
    int size;				//文件大小
    int fatherBlockNum;		//当前的父目录盘块号
    int currentBlockNum;    //当前的盘块
    void initialize();
};
void FCB::initialize() {
    strcpy(fname, "\0");
    type = Zero;
    size = 0;
    fatherBlockNum = currentBlockNum = 0;
}
const char* FilePath = "D:/myfiles";	//常量设置
const int BlockSize = 512;				//盘块大小
const int OPEN_MAX = 5;					//能打开最多的文件数
const int BlockCount = 128;				 //盘块数
const int DiskSize = BlockSize * BlockCount;    //磁盘大小

int OpenFileCount = 0;			// 统计当前打开文件数目
const int BlockFcbCount = BlockSize / sizeof(FCB);	//目录文件的最多FCB数
struct OPENLIST				//用户文件打开表
{
    int files;				//当前打开文件数
    FCB f[OPEN_MAX];		//FCB拷贝
    OPENLIST();
};
OPENLIST::OPENLIST() {
    files = 0;
    for (int i = 0; i < OPEN_MAX; i++) {
        f[i].fatherBlockNum = -1;	//为分配打开
        f[i].type = GENERAL;
    }
}
struct dirFile		//目录结构
{
    struct FCB fcb[BlockFcbCount];
    void init(int _FatherBlockNum, int _CurrentBlockNum,const char* name)	//父块号，当前块号，目录名
    {
        strcpy(fcb[0].fname, name);			//本身的FCB
        fcb[0].fatherBlockNum = _FatherBlockNum;
        fcb[0].currentBlockNum = _CurrentBlockNum;
        fcb[0].type = DIRECTORY;			//标记目录文件
        for (int i = 1; i < BlockFcbCount; i++) {
            fcb[i].fatherBlockNum = _CurrentBlockNum;	//标记为子项
            fcb[i].type = Zero;				// 标记为空白项
        }
    }
};
struct DISK {
    int FAT1[BlockCount];     //FAT1
    int FAT2[BlockCount];     //FAT2
    struct dirFile root;    //根目录
    char data[BlockCount - 3][BlockSize];
    void format();
};
void DISK::format() {
    memset(FAT1, 0, BlockCount);		//FAT1
    memset(FAT2, 0, BlockCount);		//FAT2
    FAT1[0] = FAT1[1] = FAT1[2] = -2;	//0,1,2盘块号依次代表FAT1,FAT2,根目录区
    FAT2[0] = FAT2[1] = FAT2[2] = -2;	//FAT作备份
    root.init(2, 2, "D:\\");			//根目录区
    memset(data, 0, sizeof(data));		//数据区
}
FILE* fp;			//磁盘文件地址
char* BaseAddr;     //虚拟磁盘空间基地址
string currentPath = "D:\\";   //当前路径
int current = 2;    //当前目录的盘块号
string cmd;         //输入指令
struct DISK* osPoint=new DISK;    //磁盘操作系统指针
char command[16];        //文件名标识
struct OPENLIST* openlist= new OPENLIST; //用户文件列表指针


int fileSystem::format() {	//格式化
    current = 2;
    currentPath = "D:\\";   //当前路径
    osPoint->format();      //打开文件列表初始化


    //保存到D:/myfiles
    fp = fopen(FilePath, "w+");
    std::fwrite(BaseAddr, sizeof(char), DiskSize, fp);
    fclose(fp);
    messageBox("格式化成功!");
    return 1;
}

int  fileSystem::mkdir(const char* sonfname)		//创建子目录
{	//判断是否有重名寻找空白子目录项寻找空白盘块号当前目录下增加该子目录项分配子目录盘块，并且初始化修改fat表
    int i, temp, iFAT;
    struct dirFile* dir=new dirFile;    //当前目录的指针
    if (current == 2)		// 根目录
        dir = &(osPoint->root);
    else
        dir = (struct dirFile*)(osPoint->data[current - 3]);
    //避免目录下同名文件夹
    for (i = 1; i < BlockFcbCount; i++) {
        if (dir->fcb[i].type == DIRECTORY && strcmp(dir->fcb[i].fname, sonfname) == 0) {
            messageBox("该文件夹下已经有同名的文件夹存在了!");
            return 0;
        }
    }
    for (i = 1; i < BlockFcbCount; i++) {	//查找空白fcb序号
        if (dir->fcb[i].type == Zero)
            break;
    }
    if (i == BlockFcbCount) {
        messageBox("该目录已满!请选择新的目录下创建!");
        return 0;
    }
    temp = i;
    for (i = 3; i < BlockCount; i++) {
        if (osPoint->FAT1[i] == 0)
            break;
    }
    if (i == BlockCount) {
        messageBox("磁盘已满!");
        return 0;
    }
    iFAT = i;
    osPoint->FAT1[iFAT] = osPoint->FAT2[iFAT] = 2;   //2表示分配给下级目录文件
                                                     //填写该分派新的盘块的参数
    strcpy(dir->fcb[temp].fname, sonfname);
    dir->fcb[temp].type = DIRECTORY;
    dir->fcb[temp].fatherBlockNum = current;
    dir->fcb[temp].currentBlockNum = iFAT;
    //初始化子目录文件盘块
    dir = (struct dirFile*)(osPoint->data[iFAT - 3]);   //定位到子目录盘块号
    dir->init(current, iFAT, sonfname);					//iFAT是要分配的块号，这里的current用来指要分配的块的父块号
    messageBox("创建子目录成功！");
    return 1;
}

int fileSystem::rmdir(const char* sonfname)   //删除当前目录下的文件夹
{

    int i, temp, j;						//确保当前目录下有该文件,并记录下该FCB下标
    struct dirFile* dir=new dirFile;				//当前目录的指针
    if (current == 2)
        dir = &(osPoint->root);
    else
        dir = (struct dirFile*)(osPoint->data[current - 3]);
    for (i = 1; i < BlockFcbCount; i++) {     //查找该目录文件
        if (dir->fcb[i].type == DIRECTORY && strcmp(dir->fcb[i].fname, sonfname) == 0) {
            break;
        }
    }
    temp = i;
    if (i == BlockFcbCount) {
        messageBox("当前目录下不存在该子目录!");
        return 0;
    }
    j = dir->fcb[temp].currentBlockNum;
    struct dirFile* sonDir=new dirFile;					//当前子目录的指针
    sonDir = (struct dirFile*)(osPoint->data[j - 3]);
    for (i = 1; i < BlockFcbCount; i++) {	//查找子目录是否为空目录
        if (sonDir->fcb[i].type != Zero) {
            messageBox("该文件夹为非空文件夹,为确保安全，请清空后再删除!");
            return 0;
        }
    }
    //开始删除子目录操作
    osPoint->FAT1[j] = osPoint->FAT2[j] = 0;    //fat清空
    char* p = osPoint->data[j - 3];				//格式化子目录
    memset(p, 0, BlockSize);
    dir->fcb[temp].initialize();				//回收子目录占据目录项
    messageBox("删除当前目录下的文件夹成功!");
    return 1;
}
int fileSystem::create(const char* name) {		//创建文本文件
    int i, iFAT;//temp,
    int emptyNum = 0, isFound = 0;			//空闲目录项个数
    struct dirFile* dir=new dirFile;					//当前目录的指针
    if (current == 2)
        dir = &(osPoint->root);
    else
        dir = (struct dirFile*)(osPoint->data[current - 3]);
    for (i = 1; i < BlockFcbCount; i++)		//查看目录是否已满//为了避免同名的文本文件
    {
        if (dir->fcb[i].type == Zero && isFound == 0) {
            emptyNum = i;
            isFound = 1;
        } else if (dir->fcb[i].type == GENERAL && strcmp(dir->fcb[i].fname, name) == 0) {
            messageBox("无法在同一目录下创建同名文件!");
            return 0;
        }
    }
    if (emptyNum == 0) {
        messageBox("已经达到目录项容纳上限，无法创建新目录!");
        return 0;
    }
    for (i = 3; i < BlockCount; i++)		//查找FAT表寻找空白区，用来分配磁盘块号j
    {
        if (osPoint->FAT1[i] == 0)
            break;
    }
    if (i == BlockCount) {
        messageBox("磁盘已满！");
        return 0;
    }
    iFAT = i;
    //进入分配阶段分配磁盘块
    osPoint->FAT1[iFAT] = osPoint->FAT2[iFAT] = 1;
    //进行分配
    strcpy(dir->fcb[emptyNum].fname, name);
    dir->fcb[emptyNum].type = GENERAL;
    dir->fcb[emptyNum].fatherBlockNum = current;
    dir->fcb[emptyNum].currentBlockNum = iFAT;
    dir->fcb[emptyNum].size = 0;
    char* p = osPoint->data[iFAT - 3];
    memset(p, 4, BlockSize);
    messageBox("在当前目录下创建文本文件成功!");
    return 1;
}
int fileSystem::listshow() {	//查询目录
    int i, DirCount = 0, FileCount = 0;
    //搜索当前目录
    struct dirFile* dir=new dirFile;     //当前目录的指针
    if (current == 2)
        dir = &(osPoint->root);
    else
        dir = (struct dirFile*)(osPoint->data[current - 3]);

    for (i = 1; i < BlockFcbCount; i++) {
        if (dir->fcb[i].type == GENERAL) {		 //查找普通文件
            FileCount++;
            drawFile(FileCount, dir->fcb[i].fname);		 //绘制文件
        }
        if (dir->fcb[i].type == DIRECTORY) {	 //查找目录文件
            DirCount++;
            drawDir(DirCount, dir->fcb[i].fname);		 //绘制文件夹
        }
    }
    return 1;
}
int fileSystem::delfile(const char* name) {		//在当前目录下删除文件
    int i, temp, j;
    //确保当前目录下有该文件,并且记录下它的FCB下标
    struct dirFile* dir=new dirFile;     //当前目录的指针
    if (current == 2)
        dir = &(osPoint->root);
    else
        dir = (struct dirFile*)(osPoint->data[current - 3]);
    for (i = 1; i < BlockFcbCount; i++) {		//查找该文件
        if (dir->fcb[i].type == GENERAL && strcmp(dir->fcb[i].fname, name) == 0) {
            break;
        }
    }
    if (i == BlockFcbCount) {
        messageBox("当前目录下不存在该文件!");
        return 0;
    }
    int k;
    for (k = 0; k < OPEN_MAX; k++) {
        if ((openlist->f[k].type == GENERAL) &&
            (strcmp(openlist->f[k].fname, name) == 0)) {
            if (openlist->f[k].fatherBlockNum == current) {
                break;
            } else {
                messageBox("该文件未在当前目录下!");
                return 0;
            }
        }
    }
    if (k != OPEN_MAX) {
        close(name);
    }
    //从打开列表中删除
    temp = i;
    j = dir->fcb[temp].currentBlockNum;    //查找盘块号j
    osPoint->FAT1[j] = osPoint->FAT2[j] = 0;     //fat1,fat2表标记为空白
    char* p = osPoint->data[j - 3];
    memset(p, 0, BlockSize);		//清除原文本文件的内容
    dir->fcb[temp].initialize();    //初始化
    messageBox("删除成功");
    return 1;
}
//进入目录
int fileSystem::changePath(const char* sonfname) {
    struct dirFile* dir=new dirFile;     //当前目录的指针
    if (current == 2)
        dir = &(osPoint->root);
    else
        dir = (struct dirFile*)(osPoint->data[current - 3]);
    //回到父目录
    if (strcmp(sonfname, "..") == 0) {
        if (current == 2) {
            messageBox("你现已经在根目录下!");
            return 0;
        }
        current = dir->fcb[0].fatherBlockNum;
        currentPath = currentPath.substr(0, currentPath.size() - strlen(dir->fcb[0].fname) - 1);
        return 1;
    }
    //进入子目录,确保当前目录下有该目录,并且记录下它的FCB下标
    int i, temp;
    for (i = 1; i < BlockFcbCount; i++) {     //查找该文件
        if (dir->fcb[i].type == DIRECTORY && strcmp(dir->fcb[i].fname, sonfname) == 0) {
            temp = i;
            break;
        }
    }
    if (i == BlockFcbCount) {
        messageBox("不存在该目录!");
        return 0;
    }
    //修改当前文件信息
    current = dir->fcb[temp].currentBlockNum;
    currentPath = currentPath + dir->fcb[temp].fname + "\\";
    messageBox("进入当前目录下的子目录成功！");
    return 1;
}
int fileSystem::save() {
    //保存到磁盘上D:\myfiles
    //将所有文件都关闭
    fp = fopen(FilePath, "w+");
    fwrite(BaseAddr, sizeof(char), DiskSize, fp);
    fclose(fp);
    //释放内存上的虚拟磁盘
    free(osPoint);
    //释放用户打开文件表
    delete openlist;
    return 1;
}
int fileSystem::write(const char* name,const char* content) {//在指定的文件里记录信息
    int i;
    char* startPoint, * endPoint;
    //在打开文件列表中查找 file(还需要考虑同名不同目录文件的情况!!!)
    for (i = 0; i < OPEN_MAX; i++) {
        if (strcmp(openlist->f[i].fname, name) == 0) {
            if (openlist->f[i].fatherBlockNum == current) {
                break;
            } else {
                messageBox("该文件处于打开列表中!");
                return 0;
            }
        }
    }
    if (i == OPEN_MAX) {
        messageBox("该文件尚未打开,请先打开后写入信息!");
        return 0;
    }
    int active = i;
    int fileStartNum = openlist->f[active].currentBlockNum - 3;
    startPoint = osPoint->data[fileStartNum];
    endPoint = osPoint->data[fileStartNum + 1];
    for (int i = 0; content[i] != '\0'; i++) {
        if (startPoint < endPoint - 1) {
            *startPoint++ = content[i];
        } else {
            messageBox("超出单体文件最大容量！");
            *startPoint++ = 4;
            break;
        }
    }
    messageBox("成功修改文件！");
    return 1;
}
int fileSystem::read(const char* file, string* content, int* size) {
    //选择一个打开的文件读取信息

    int i, fileStartNum;
    char* startPoint, * endPoint;
    //struct dirFile *dir;
    //在打开文件列表中查找 file(还需要考虑同名不同目录文件的情况!!!)
    for (i = 0; i < OPEN_MAX; i++) {
        if (strcmp(openlist->f[i].fname, file) == 0) {
            if (openlist->f[i].fatherBlockNum == current) {
                break;
            } else {
                messageBox("无法打开同名文件！");
                return 0;
            }
        }
    }
    if (i == OPEN_MAX) {
        messageBox("打开文件数量达到上限！");
        return 0;
    }
    int active = i;//计算文件物理地址
    fileStartNum = openlist->f[active].currentBlockNum - 3;
    startPoint = osPoint->data[fileStartNum];
    endPoint = osPoint->data[fileStartNum + 1];
    while ((*startPoint) != 4 && (startPoint < endPoint)) {
        content->push_back(*startPoint++);
    }
    *size = content->size();
    return 1;
}

int fileSystem::open(const char* file) {  //打开文件
    int i, FcbIndex;
    //确保没有打开过该文件 = 相同名字 + 相同目录
    for (i = 0; i < OPEN_MAX; i++) {
        if (openlist->f[i].type == GENERAL && strcmp(openlist->f[i].fname, file) == 0
            && openlist->f[i].fatherBlockNum == current) {
            messageBox("该文件已经被打开!");
            return 0;
        }
    }
    //确保有空的打开文件项
    if (openlist->files == OPEN_MAX) {
        messageBox("打开文件数目达到上限!");
        return 0;
    }
    //确保当前目录下有该文件,并且记录下它的FCB下标
    struct dirFile* dir;     //当前目录的指针
    if (current == 2)
        dir = &(osPoint->root);
    else
        dir = (struct dirFile*)(osPoint->data[current - 3]);

    for (i = 1; i < BlockFcbCount; i++) {     //查找该文件
        if (dir->fcb[i].type == GENERAL && strcmp(dir->fcb[i].fname, file) == 0) {
            FcbIndex = i;
            break;
        }
    }
    if (i == BlockFcbCount) {
        return 0;
    }
    openlist->f[OpenFileCount] = dir->fcb[FcbIndex]; //FCB拷贝
    openlist->files++;
    OpenFileCount++;
    return 1;
}
int fileSystem::close(const char* file) {
    //释放该文件所占内存//释放用户打开文件列表表项
    int i;
    //在打开文件列表中查找 file
    for (i = 0; i < OPEN_MAX; i++) {
        if ((openlist->f[i].type == GENERAL) &&
            (strcmp(openlist->f[i].fname, file) == 0)) {
            if (openlist->f[i].fatherBlockNum == current) {
                break;
            } else {
                messageBox("无法关闭非当前目录下的文件");
                return 0;
            }
        }
    }
    if (i == OPEN_MAX) {
        messageBox("该文件未被打开");
        return 0;
    }
    int active = i;
    openlist->files--;
    openlist->f[active].initialize();
    OpenFileCount--;
    //messageBox("成功关闭文件");

    return 1;
}
int fileSystem::close2(const char* file,const char* content) {
    //释放该文件所占内存//释放用户打开文件列表表项
    int i;
    //在打开文件列表中查找 file
    for (i = 0; i < OPEN_MAX; i++) {
        if ((openlist->f[i].type == GENERAL) &&
            (strcmp(openlist->f[i].fname, file) == 0)) {
            if (openlist->f[i].fatherBlockNum == current) {
                break;
            } else {
                messageBox("无法关闭非当前目录下的文件");
                QMessageBox* message = new QMessageBox(ui->centralWidget);
                message->setModal(false);
                message->setText(content);
                message->setWindowTitle("文件内容");
                message->setButtonText(1, "关闭");
                message->setAttribute(Qt::WA_DeleteOnClose);
                message->show();
                if (message->exec()) {
                    close(file);
                }
                return 0;
            }
        }
    }
    if (i == OPEN_MAX) {
        messageBox("该文件未被打开");
        return 0;
    }
    int active = i;
    openlist->files--;
    openlist->f[active].initialize();
    OpenFileCount--;
    //messageBox("成功关闭文件");
    return 1;
}
bool fileSystem::canClose(const char* file) {
    //释放该文件所占内存//释放用户打开文件列表表项
    int i;
    //在打开文件列表中查找 file
    for (i = 0; i < OPEN_MAX; i++) {
        if ((openlist->f[i].type == GENERAL) &&
            (strcmp(openlist->f[i].fname, file) == 0)) {
            if (openlist->f[i].fatherBlockNum == current) {
                break;
            } else {
                messageBox("无法关闭非当前目录下的文件");
                return 0;
            }
        }
    }
    if (i == OPEN_MAX) {
        messageBox("该文件未被打开");
        return 0;
    }
    return 1;
}

fileSystem::fileSystem(QWidget* parent)
    : QMainWindow(parent) {
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);    // 禁止最大化按钮
    setFixedSize(this->width(), this->height());                     // 禁止拖动窗口大小
    dirLayout = new QVBoxLayout();
    fileLayout = new QVBoxLayout();
    sizeLayout = new QVBoxLayout();
    pathLayout = new QHBoxLayout();
    Initial(0);


    //connect
    connect(ui->saveBtn,&QPushButton::clicked,this,&fileSystem::saveAll);
    connect(ui->pushButton,&QPushButton::clicked,this,&fileSystem::formatAll);
    connect(ui->preDirBtn,&QPushButton::clicked,this,&fileSystem::backDir);
    connect(ui->addFileBtn,&QPushButton::clicked,this,&fileSystem::addFile);
    connect(ui->addDirBtn,&QPushButton::clicked,this,&fileSystem::addDirFuc);
}
void fileSystem::addDirFuc() {
    bool isOK;
    QString text = QInputDialog::getText(NULL, "输入目录名...",
        "请输入要创建的目录名",
        QLineEdit::Normal,
        "目录",
        &isOK);
    if (isOK) {
        //增加文件
        string strText = text.toStdString();
        char* dirName = (char*)strText.c_str();
        mkdir(dirName);
    }
    listshow();
}
void fileSystem::addFile() {
    bool isOK;
    QString text = QInputDialog::getText(NULL, "输入文件名...",
        "请输入要创建的文件名",
        QLineEdit::Normal,
        "文件",
        &isOK);
    if (isOK) {
        //增加文件
        string strText = text.toStdString();
        char* fileName = (char*)strText.c_str();
        create(fileName);
    }
    listshow();
}
void fileSystem::backDir() {
    if (changePath("..")) {
        QLayoutItem* child1 = pathLayout->takeAt(pathLayout->count() - 1);
        //setParent为NULL，防止删除之后界面不消失
        if (child1->widget()) {
            child1->widget()->setParent(NULL);
        }
        delete child1;
    };
    updateLayout();
    listshow();
}
void fileSystem::formatAll() {
    updateLayout();
    format();
}
void fileSystem::drawDir(int i,const char* dirName) {
    QPushButton* dirBtn = new QPushButton(ui->centralWidget);
    dirBtn->setGeometry(QRect(300, 40 + 50 * i, 75, 41));
    dirBtn->setIcon(QIcon("./Resources/res.qrc/new.prefix1/res/folder.png"));
    dirBtn->setText(dirName);
    connect(dirBtn, QOverload<bool>::of(&QPushButton::clicked),
        [=]() {
        QStringList items; //ComboBox 列表的内容
        items << "进入" << "删除";
        QString dlgTitle = "选择";
        QString txtLabel = "请选择操作";
        int     curIndex = 0;		//初始选择项
        bool    editable = true;	//ComboBox是否可编辑
        bool    ok = false;
        QString text = QInputDialog::getItem(this, dlgTitle, txtLabel, items, curIndex, editable, &ok);
        if (ok && !text.isEmpty()) {
            if (text == "进入") {
                changePath(dirName);		//更改路径 下面是绘制路径
                QLabel* pathLabel = new QLabel(ui->centralWidget);
                pathLabel->setGeometry(QRect(130 + 40 * pathLayout->count(), 68, 40, 16));
                const char* a = "/";
                char* t = new char[strlen(dirName) + strlen(a) + 1];
                strcpy(t, dirName);
                strcat(t, a);
                pathLabel->setText(t);
                pathLayout->addWidget(pathLabel);
                pathLabel->show();
                updateLayout();
                listshow();
            }
            if (text == "删除") {
                rmdir(dirName);
                dirLayout->removeWidget(dirBtn);
                delete dirBtn;
                updateLayout();
                listshow();
            }
        }
    });
    dirLayout->addWidget(dirBtn);
    dirBtn->show();
}
void fileSystem::drawFile(int i, const char* fileName) {
    QLabel* fileSize = new QLabel(ui->centralWidget);
    fileSize->setGeometry(520, 40 + 50 * i, 75, 41);
    int size = 0;
    string content;
    open(fileName);
    read(fileName, &content, &size);
    close(fileName);
    fileSize->setText(QString::fromStdString(to_string(size)).append("b"));
    QPushButton* fileBtn = new QPushButton(ui->centralWidget);
    fileBtn->setGeometry(QRect(420, 40 + 50 * i, 75, 41));		//设置位置，大小
    fileBtn->setText(fileName);									//按钮文字
    connect(fileBtn, QOverload<bool>::of(&QPushButton::clicked),
        [=]() {
        QStringList items; //ComboBox 列表的内容
        items <<"查看"<< "修改"<< "删除";
        QString dlgTitle = "选择";
        QString txtLabel = "请选择操作";
        int     curIndex = 0;			//初始选择项
        bool    editable = true;		//ComboBox是否可编辑
        bool    ok = false;
        QString text = QInputDialog::getItem(this, dlgTitle, txtLabel, items, curIndex, editable, &ok);
        if (ok && !text.isEmpty()) {
            if (text == "查看") {
                string defaultInput = "";
                if (open(fileName)) {
                    int size;
                    if (read(fileName, &defaultInput, &size)) {
                        QMessageBox* message = new QMessageBox(ui->centralWidget);
                        message->setModal(false);
                        message->setText(defaultInput.c_str());
                        message->setWindowTitle("文件内容");
                        message->setButtonText(1, "关闭");
                        message->setAttribute(Qt::WA_DeleteOnClose);
                        message->show();
                        if (message->exec()) {
                            close2(fileName, (char*)defaultInput.c_str());
                            updateLayout();
                            listshow();
                        }
                    }
                }
            }
            if (text == "修改") {					//修改文件
                string defaultInput = "";
                if (open(fileName)) {						//如果没有被打开
                    int size;								//文件大小
                    read(fileName, &defaultInput, &size);
                    bool bOk = false;
                    QString text = QInputDialog::getMultiLineText(this,		//弹出输入文本框
                        "修改..",
                        "请输入内容",
                        defaultInput.c_str(),
                        &bOk
                    );
                    if (bOk) {								//点击确定
                        write(fileName, (char*)text.toStdString().c_str());		//修改文件内容
                    }
                    close(fileName);						//关闭文件
                    updateLayout();
                    listshow();
                }
            }
            if (text == "删除") {					//删除文件
                delfile(fileName);
                fileLayout->removeWidget(fileBtn);			//删除按钮
                delete fileBtn;
                updateLayout();								//更新布局
                listshow();									//更新列表
            }
        }
    });
    fileLayout->addWidget(fileBtn);
    sizeLayout->addWidget(fileSize);
    fileSize->show();
    fileBtn->show();
}
void fileSystem::messageBox(const char* s) {
    QMessageBox::information(NULL, "提示", s);
}
void fileSystem::updateLayout() {
    QLayoutItem* child1;
    QLayoutItem* child2;
    QLayoutItem* child3;
    while ((child1 = fileLayout->takeAt(0)) != 0) {		//删除布局中的items
        //setParent为NULL，防止删除之后界面不消失
        if (child1->widget()) {
            child1->widget()->setParent(NULL);
        }
        delete child1;
    }
    while ((child2 = dirLayout->takeAt(0)) != 0) {
        //setParent为NULL，防止删除之后界面不消失
        if (child2->widget()) {
            child2->widget()->setParent(NULL);
        }
        delete child2;
    }
    while ((child3 = sizeLayout->takeAt(0)) != 0) {
        //setParent为NULL，防止删除之后界面不消失
        if (child3->widget()) {
            child3->widget()->setParent(NULL);
        }
        delete child3;
    }
}
void fileSystem::Initial(int i) {
    openlist = new OPENLIST;							//创建用户文件打开表
    BaseAddr = (char*)malloc(DiskSize);					//申请虚拟空间并且初始化
    osPoint = (struct DISK*)(BaseAddr);					//虚拟磁盘初始化
    if ((fp = fopen(FilePath, "r")) != NULL) {			//加载磁盘文件
        fread(BaseAddr, sizeof(char), DiskSize, fp);
        i ? messageBox("保存成功!") : messageBox("欢迎回来!");
    } else {
        format();
        messageBox("初始化已经完成!");
    }
    updateLayout();
    listshow();
}
void fileSystem::saveAll() {
    save();
    Initial(1);
}


fileSystem::~fileSystem()
{
    delete ui;
}
