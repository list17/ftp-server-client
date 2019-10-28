import sys
import time

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from login import Ui_MainWindow as LoginPage
from address_settings import Ui_Dialog as AddrSetPage
from file import Ui_ListFTP as FilePage
from renameWindow import Ui_Dialog as RenamePage
from mkdirWindow import Ui_Dialog as MkdirPage
from downloadProBarPage import Ui_Dialog as DownloadPage
from ftp_client import FTPClient

class BasicVar:
    def __init__(self):
        self.login_page = LoginPage()
        self.addr_set_page = AddrSetPage()
        self.rename_page = RenamePage()
        self.download_page = DownloadPage()
        self.mkdir_page = MkdirPage()
        self.file_page = FilePage()


global_var = BasicVar()
client = FTPClient()


def main():
    app = QApplication(sys.argv)
    login_page = QMainWindow()
    global_var.login_page.setupUi(login_page)
    login_page.show()
    global_var.login_page.confirm_btn.clicked.connect(lambda: confirm_btn_clicked(login_page))
    global_var.login_page.exit_btn.clicked.connect(lambda: exit_btn_clicked(login_page))
    global_var.login_page.menu.triggered[QAction].connect(lambda: set_address(global_var.login_page.actionaddress_settings))
    sys.exit(app.exec_())


#确定按钮点击函数
def confirm_btn_clicked(main_window):
    client.username = global_var.login_page.username_edit.text()
    client.password = global_var.login_page.password_edit.text()
    if client.username != "anonymous":
        msg = QMessageBox(QMessageBox.Critical, "anonymous", "error")
        msg.exec_()
    else:
        if client.cmd_user() and client.cmd_pass():
            global_var.file_page.setupUi(main_window)
            msg = QMessageBox(QMessageBox.Information, "登录成功", "success")
            msg.exec_()
            if not client.cmd_pwd():
                msg = QMessageBox(QMessageBox.Critical, "获取目录失败", "error")
                msg.exec_()
            create_list()
            global_var.file_page.listWidget.setContextMenuPolicy(Qt.CustomContextMenu)
            global_var.file_page.listWidget.customContextMenuRequested[QPoint].connect(listWidgetContext)
            global_var.file_page.listWidget.doubleClicked.connect(lambda: handle_cmd())
            global_var.file_page.return_btn.clicked.connect(lambda: handle_return_btn_clicked())
            global_var.file_page.create_btn.clicked.connect(lambda: handle_mkdir())
            global_var.file_page.upload_btn.clicked.connect(lambda: handle_upload_btn_clicked(main_window))
            global_var.file_page.begin_upload_btn.clicked.connect(lambda: handle_begin_upload_btn_clicked())


#退出按钮点击函数
def exit_btn_clicked(main_window):
    main_window.close()


#设定port确定按钮点击函数
def addr_set_confirm_btn_clicked():
    client.address = global_var.addr_set_page.address_edit.text()
    client.port = global_var.addr_set_page.port_edit.text()
    msg = QMessageBox()
    msg.setIcon(QMessageBox.Information)
    msg.setText("设定成功")
    msg.setWindowTitle("succsee")
    msg.exec_()


#设定port退出按钮点击函数
def addr_set_exit_btn_clicked(dialog):
    dialog.close()


def set_address(action):
    if action.text() == "address settings":
        addr_set_page = QDialog()
        global_var.addr_set_page.setupUi(addr_set_page)
        addr_set_page.show()
        global_var.addr_set_page.addr_confirm_btn.clicked.connect(lambda: addr_set_confirm_btn_clicked())
        global_var.addr_set_page.addr_exit_btn.clicked.connect(lambda: addr_set_exit_btn_clicked(addr_set_page))
        addr_set_page.exec_()


def handle_download():
    download_page = QDialog()
    global_var.download_page.setupUi(download_page)
    download_page.show()
    global_var.download_page.begin_btn.clicked.connect(lambda: download_btn_clicked(download_page))
    download_page.exec_()


def download_btn_clicked(dialog):
    client.cmd_retr(global_var.file_page.listWidget.currentItem().text()[
                      global_var.file_page.listWidget.currentItem().text().find(':') + 4:],
                    open(global_var.file_page.listWidget.currentItem().text()[
                      global_var.file_page.listWidget.currentItem().text().find(':') + 4:], 'wb').write,
                    global_var.download_page)
    # sum = 10000
    # global_var.download_page.sum_label.setText(str(sum))
    # global_var.download_page.sum_label.timerEvent()
    # a = 0
    # while a != sum:
    #     global_var.download_page.lineEdit.setText(str(a))
    #     dialog.update()
    #     a += 100
    #     time.sleep(0.1)
    dialog.close()


def handle_rename():
    rename_page = QDialog()
    global_var.rename_page.setupUi(rename_page)
    rename_page.show()
    global_var.rename_page.buttonBox.accepted.connect(lambda: rename_confirm_btn_clicked())
    global_var.rename_page.buttonBox.rejected.connect(lambda: rename_exit_btn_clicked(rename_page))
    rename_page.exec_()


def rename_confirm_btn_clicked():
    client.cmd_rename(global_var.file_page.listWidget.currentItem().text()[global_var.file_page.listWidget.currentItem().text().find(':') + 4:], global_var.rename_page.lineEdit.text())
    create_list()
    # print(global_var.rename_page.lineEdit.text())


def rename_exit_btn_clicked(dialog):
    dialog.close()


def handle_delete(delete):
    file = global_var.file_page.listWidget.currentItem().text()[
                      global_var.file_page.listWidget.currentItem().text().find(':') + 4:]
    if delete == 1:
        #文件夹
        if client.cmd_rmd(file):
            msg = QMessageBox(QMessageBox.Information, "Success", "删除空文件夹成功")
            msg.exec_()
    elif delete == 2:
        #文件
        if client.cmd_delete(file):
            msg = QMessageBox(QMessageBox.Information, "Success", "删除文件成功")
            msg.exec_()
    create_list()


def handle_mkdir():
    mkdir_page = QDialog()
    global_var.mkdir_page.setupUi(mkdir_page)
    mkdir_page.show()
    global_var.mkdir_page.buttonBox.accepted.connect(lambda: mkdir_confirm_btn_clicked())
    global_var.mkdir_page.buttonBox.rejected.connect(lambda: mkdir_exit_btn_clicked(mkdir_page))
    mkdir_page.exec_()


def mkdir_confirm_btn_clicked():
    if client.cmd_mkd(global_var.mkdir_page.lineEdit.text()):
        msg = QMessageBox(QMessageBox.Information, "Success", "创建文件夹成功")
        msg.exec_()
    create_list()


def mkdir_exit_btn_clicked(dialog):
    dialog.close()


def listWidgetContext(point):
    print(global_var.file_page.listWidget.currentItem().text())
    popMenu = QMenu()
    if global_var.file_page.listWidget.currentItem().text()[0] == 'd':
        rename = popMenu.addAction("重命名")
        delete = popMenu.addAction("删除该文件夹")
        rename.triggered.connect(lambda: handle_rename())
        delete.triggered.connect(lambda: handle_delete(1))
        popMenu.exec_(QCursor.pos())
    else:
        download = popMenu.addAction("下载")
        rename = popMenu.addAction("重命名")
        delete = popMenu.addAction("删除该文件")
        download.triggered.connect(lambda: handle_download( ))
        rename.triggered.connect(lambda: handle_rename())
        delete.triggered.connect(lambda: handle_delete(2))
        popMenu.exec_(QCursor.pos())


def create_list():
    client.directory = []
    client.cmd_list()
    global_var.file_page.listWidget.clear()
    for item in client.directory:
        a = QListWidgetItem(' '.join(item))
        if item[0]:
            if(item[0][0] == 'd'):
                a.setTextColor(Qt.white)
            else:
                a.setTextColor(Qt.blue)
            global_var.file_page.listWidget.addItem(a)


def handle_cmd():
    if global_var.file_page.listWidget.currentItem().text()[0] == 'd':
        client.cmd_cwd(global_var.file_page.listWidget.currentItem().text()[
                      global_var.file_page.listWidget.currentItem().text().find(':') + 4:])
    create_list()


def handle_return_btn_clicked():
    client.cmd_cwd('../')
    create_list()


def handle_upload_btn_clicked(widget):
    # absolute_path is a QString object
    absolute_path = QFileDialog.getOpenFileName(widget, "Open file", ".", "files(*.*)", )

    if absolute_path:
        cur_path = QDir('.')
        relative_path = cur_path.relativeFilePath(absolute_path)
        client.upload_filename = absolute_path
        global_var.file_page.lineEdit.setText(relative_path)


def handle_begin_upload_btn_clicked():
    pass


if __name__ == '__main__':
    main()
    # client.username = "anonymous"
    # client.cmd_user()
    # client.cmd_pass()
    # client.cmd_syst()
    # client.cmd_list(print())
    # client.cmd_syst()