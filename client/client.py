import sys

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from login import Ui_MainWindow as LoginPage
from address_settings import Ui_Dialog as AddrSetPage
from file import Ui_ListFTP as FilePage
from renameWindow import Ui_Dialog as RenamePage
from ftp_client import FTPClient

class BasicVar:
    def __init__(self):
        self.login_page = LoginPage()
        self.addr_set_page = AddrSetPage()
        self.rename_page = RenamePage()
        self.file_page = FilePage()


class Command:
    def __init__(self):
        pass


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
            client.cmd_list()
            for item in client.directory:
                a = QListWidgetItem(' '.join(item))
                global_var.file_page.listWidget.addItem(a)
            global_var.file_page.listWidget.setContextMenuPolicy(Qt.CustomContextMenu)
            global_var.file_page.listWidget.customContextMenuRequested[QPoint].connect(listWidgetContext)


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


def handle_download(action):
    print(action.text())
    get_filename = global_var.file_page.listWidget.currentItem().text().split(' ')[-1]
    client.cmd_retr(get_filename, open(get_filename, 'wb').write)


def handle_rename(action):
    rename_page = QDialog()
    global_var.rename_page.setupUi(rename_page)
    rename_page.show()
    global_var.rename_page.buttonBox.accepted.connect(lambda: rename_confirm_btn_clicked())
    global_var.rename_page.buttonBox.rejected.connect(lambda: rename_exit_btn_clicked(rename_page))
    rename_page.exec_()


def rename_confirm_btn_clicked():
    print(global_var.rename_page.lineEdit.text())


def rename_exit_btn_clicked(dialog):
    dialog.close()


def handle_delete(action):
    print(action.text())


def listWidgetContext(point):
    print(global_var.file_page.listWidget.currentItem().text())
    popMenu = QMenu()
    download = popMenu.addAction("下载")
    rename = popMenu.addAction("重命名")
    delete = popMenu.addAction("删除该文件")
    download.triggered.connect(lambda: handle_download(download))
    rename.triggered.connect(lambda: handle_rename(rename))
    delete.triggered.connect(lambda: handle_delete(delete))
    popMenu.exec_(QCursor.pos())


if __name__ == '__main__':
    main()
    # client.username = "anonymous"
    # client.cmd_user()
    # client.cmd_pass()
    # client.cmd_syst()
    # client.cmd_list(print())
    # client.cmd_syst()