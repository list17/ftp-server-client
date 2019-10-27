# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'address_settings.ui'
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

class Ui_Dialog(object):
    def setupUi(self, Dialog):
        Dialog.setObjectName(_fromUtf8("Dialog"))
        Dialog.resize(400, 300)
        self.addr_exit_btn = QtGui.QPushButton(Dialog)
        self.addr_exit_btn.setGeometry(QtCore.QRect(231, 181, 84, 38))
        self.addr_exit_btn.setObjectName(_fromUtf8("addr_exit_btn"))
        self.addr_confirm_btn = QtGui.QPushButton(Dialog)
        self.addr_confirm_btn.setGeometry(QtCore.QRect(96, 181, 84, 38))
        self.addr_confirm_btn.setObjectName(_fromUtf8("addr_confirm_btn"))
        self.layoutWidget = QtGui.QWidget(Dialog)
        self.layoutWidget.setGeometry(QtCore.QRect(80, 50, 262, 76))
        self.layoutWidget.setObjectName(_fromUtf8("layoutWidget"))
        self.gridLayout = QtGui.QGridLayout(self.layoutWidget)
        self.gridLayout.setMargin(0)
        self.gridLayout.setObjectName(_fromUtf8("gridLayout"))
        self.label = QtGui.QLabel(self.layoutWidget)
        self.label.setObjectName(_fromUtf8("label"))
        self.gridLayout.addWidget(self.label, 0, 0, 1, 2)
        self.address_edit = QtGui.QLineEdit(self.layoutWidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.address_edit.sizePolicy().hasHeightForWidth())
        self.address_edit.setSizePolicy(sizePolicy)
        # self.address_edit.setClearButtonEnabled(False)
        self.address_edit.setObjectName(_fromUtf8("address_edit"))
        self.gridLayout.addWidget(self.address_edit, 0, 2, 1, 1)
        self.port_edit = QtGui.QLineEdit(self.layoutWidget)
        self.port_edit.setContextMenuPolicy(QtCore.Qt.NoContextMenu)
        self.port_edit.setInputMask(_fromUtf8(""))
        self.port_edit.setEchoMode(QtGui.QLineEdit.Normal)
        # self.port_edit.setClearButtonEnabled(False)
        self.port_edit.setObjectName(_fromUtf8("port_edit"))
        self.gridLayout.addWidget(self.port_edit, 1, 2, 1, 1)
        self.label_2 = QtGui.QLabel(self.layoutWidget)
        self.label_2.setObjectName(_fromUtf8("label_2"))
        self.gridLayout.addWidget(self.label_2, 1, 0, 1, 2)

        self.retranslateUi(Dialog)
        QtCore.QMetaObject.connectSlotsByName(Dialog)

    def retranslateUi(self, Dialog):
        Dialog.setWindowTitle(_translate("Dialog", "Dialog", None))
        self.addr_exit_btn.setText(_translate("Dialog", "退出", None))
        self.addr_confirm_btn.setText(_translate("Dialog", "确定", None))
        self.label.setText(_translate("Dialog", "address", None))
        self.address_edit.setText(_translate("Dialog", "127.0.0.1", None))
        self.port_edit.setText(_translate("Dialog", "21", None))
        self.label_2.setText(_translate("Dialog", "port:", None))

