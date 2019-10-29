# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'main.ui'
#
# Created by: PyQt4 UI code generator 4.12.3
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig)

class Ui_ListFTP(object):
    def setupUi(self, ListFTP):
        ListFTP.setObjectName(_fromUtf8("ListFTP"))
        ListFTP.resize(900, 600)
        self.centralwidget = QtGui.QWidget(ListFTP)
        self.centralwidget.setObjectName(_fromUtf8("centralwidget"))
        self.layoutWidget = QtGui.QWidget(self.centralwidget)
        self.layoutWidget.setGeometry(QtCore.QRect(21, 13, 861, 531))
        self.layoutWidget.setObjectName(_fromUtf8("layoutWidget"))
        self.verticalLayout = QtGui.QVBoxLayout(self.layoutWidget)
        self.verticalLayout.setMargin(0)
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        self.horizontalLayout_3 = QtGui.QHBoxLayout()
        self.horizontalLayout_3.setObjectName(_fromUtf8("horizontalLayout_3"))
        self.return_btn = QtGui.QPushButton(self.layoutWidget)
        self.return_btn.setObjectName(_fromUtf8("return_btn"))
        self.horizontalLayout_3.addWidget(self.return_btn)
        self.create_btn = QtGui.QPushButton(self.layoutWidget)
        self.create_btn.setObjectName(_fromUtf8("create_btn"))
        self.horizontalLayout_3.addWidget(self.create_btn)
        spacerItem = QtGui.QSpacerItem(80, 20, QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout_3.addItem(spacerItem)
        self.horizontalLayout = QtGui.QHBoxLayout()
        self.horizontalLayout.setObjectName(_fromUtf8("horizontalLayout"))
        self.upload_btn = QtGui.QPushButton(self.layoutWidget)
        self.upload_btn.setObjectName(_fromUtf8("upload_btn"))
        self.horizontalLayout.addWidget(self.upload_btn)
        self.lineEdit = QtGui.QLineEdit(self.layoutWidget)
        self.lineEdit.setObjectName(_fromUtf8("lineEdit"))
        self.horizontalLayout.addWidget(self.lineEdit)
        self.begin_upload_btn = QtGui.QPushButton(self.layoutWidget)
        self.begin_upload_btn.setObjectName(_fromUtf8("begin_upload_btn"))
        self.horizontalLayout.addWidget(self.begin_upload_btn)
        self.horizontalLayout_3.addLayout(self.horizontalLayout)
        self.verticalLayout.addLayout(self.horizontalLayout_3)
        self.listWidget = QtGui.QListWidget(self.layoutWidget)
        self.listWidget.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.listWidget.setObjectName(_fromUtf8("listWidget"))
        self.verticalLayout.addWidget(self.listWidget)
        ListFTP.setCentralWidget(self.centralwidget)
        self.menubar = QtGui.QMenuBar(ListFTP)
        self.menubar.setGeometry(QtCore.QRect(0, 0, 900, 32))
        self.menubar.setObjectName(_fromUtf8("menubar"))
        self.menusettings = QtGui.QMenu(self.menubar)
        self.menusettings.setObjectName(_fromUtf8("menusettings"))
        ListFTP.setMenuBar(self.menubar)
        self.statusbar = QtGui.QStatusBar(ListFTP)
        self.statusbar.setObjectName(_fromUtf8("statusbar"))
        ListFTP.setStatusBar(self.statusbar)
        self.actionset_Mode = QtGui.QAction(ListFTP)
        self.actionset_Mode.setObjectName(_fromUtf8("actionset_Mode"))
        self.menusettings.addAction(self.actionset_Mode)
        self.menubar.addAction(self.menusettings.menuAction())

        self.retranslateUi(ListFTP)
        QtCore.QMetaObject.connectSlotsByName(ListFTP)

    def retranslateUi(self, ListFTP):
        ListFTP.setWindowTitle(_translate("ListFTP", "MainWindow", None))
        self.return_btn.setText(_translate("ListFTP", "返回上层", None))
        self.create_btn.setText(_translate("ListFTP", "创建文件夹", None))
        self.upload_btn.setText(_translate("ListFTP", "选择上传文件", None))
        self.begin_upload_btn.setText(_translate("ListFTP", "上传", None))
        self.menusettings.setTitle(_translate("ListFTP", "settings", None))
        self.actionset_Mode.setText(_translate("ListFTP", "set Mode", None))

