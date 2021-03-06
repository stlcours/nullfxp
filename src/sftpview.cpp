// sftpview.cpp --- 
// 
// Author: liuguangzhao
// Copyright (C) 2007-2010 liuguangzhao@users.sf.net
// URL: http://www.qtchina.net http://nullget.sourceforge.net
// Created: 2008-05-05 21:49:36 +0800
// Version: $Id$
// 

#include <QtCore>
#include <QtGui>

#include "ssh_info.h"

#include "globaloption.h"
#include "utils.h"

#include "progressdialog.h"
#include "localview.h"
#include "sftpview.h"
#include "netdirsortfiltermodel.h"
#include "ui_remoteview.h"

#include "fileproperties.h"
#include "encryptiondetailfocuslabel.h"
#include "encryptiondetaildialog.h"
#include "systeminfodialog.h"

#ifndef _MSC_VER
#warning "wrapper lower class, drop this include"
#endif

#include "rfsdirnode.h"
#include "connection.h"

SFTPView::SFTPView(QMdiArea *main_mdi_area, LocalView *local_view, QWidget *parent)
    : RemoteView(main_mdi_area, local_view, parent)
{
    this->init_popup_context_menu();
    this->rconn = NULL;
}

SFTPView::~SFTPView()
{
}

void SFTPView::slot_show_fxp_command_log(bool show)
{
    this->uiw->listView->setVisible(show);    
}

void SFTPView::i_init_dir_view()
{
    qDebug()<<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;

    this->remote_dir_model = new RemoteDirModel();
    this->remote_dir_model->setConnection(this->conn);

    // operation log
    this->m_operationLogModel = new QStringListModel();
    this->uiw->listView->setModel(this->m_operationLogModel);
    QObject::connect(this->remote_dir_model, SIGNAL(operationTriggered(QString)), 
                     this, SLOT(slot_operation_triggered(QString)));
    QObject::connect(this->remote_dir_model, SIGNAL(directoryLoaded(const QString &)),
                     this, SLOT(onDirectoryLoaded(const QString &)));
    
    this->remote_dir_model->set_user_home_path(this->user_home_path);
    this->m_tableProxyModel = new DirTableSortFilterModel();
    this->m_tableProxyModel->setSourceModel(this->remote_dir_model);
    this->m_treeProxyModel = new DirTreeSortFilterModel();
    this->m_treeProxyModel->setSourceModel(this->remote_dir_model);
    this->uiw->treeView->setModel(this->m_treeProxyModel);
    // this->uiw->treeView->setModel(this->remote_dir_model);

    this->uiw->treeView->setAcceptDrops(true);
    this->uiw->treeView->setDragEnabled(false);
    this->uiw->treeView->setDropIndicatorShown(true);
    this->uiw->treeView->setDragDropMode(QAbstractItemView::DropOnly);

    QObject::connect(this->remote_dir_model,
                     SIGNAL(sig_drop_mime_data(const QMimeData *, Qt::DropAction, int, int, const QModelIndex &)),
                     this, SLOT(slot_drop_mime_data(const QMimeData *, Qt::DropAction, int, int, const QModelIndex &)));
    
    QObject::connect(this->remote_dir_model, SIGNAL(enter_remote_dir_retrive_loop()),
                     this, SLOT(slot_enter_remote_dir_retrive_loop()));
    QObject::connect(this->remote_dir_model, SIGNAL(leave_remote_dir_retrive_loop()),
                     this, SLOT(slot_leave_remote_dir_retrive_loop()));
    
    this->uiw->treeView->expandAll();
    this->uiw->treeView->setColumnWidth(0, this->uiw->treeView->columnWidth(0) * 2);
    
    //这里设置为true时，导致这个treeView不能正确显示滚动条了，为什么呢?
    // this->uiw->treeView->setColumnHidden(1, true);
    this->uiw->treeView->setColumnWidth(1, 0); // 使用这种方法隐藏看上去就正常了。
    this->uiw->treeView->setColumnHidden(2, true);
    this->uiw->treeView->setColumnHidden(3, true);
  
    /////tableView
    QModelIndex homeIndex = this->remote_dir_model->index(this->user_home_path);
    this->uiw->tableView->setModel(this->m_tableProxyModel);
    // this->uiw->tableView->setModel(this->remote_dir_model);
    this->uiw->tableView->setRootIndex(this->m_tableProxyModel->mapFromSource(homeIndex));
    
    //change row height of table 
    this->table_row_height = 20; // default table height
    if (this->remote_dir_model->rowCount(homeIndex) > 0) {
        this->table_row_height = this->uiw->tableView->rowHeight(0)*2/3;
    } else {
        this->table_row_height = 20 ;
    }
    for (int i = 0; i < this->remote_dir_model->rowCount(homeIndex); i ++) {
        this->uiw->tableView->setRowHeight(i, this->table_row_height);
    }
    this->uiw->tableView->resizeColumnToContents(0);

    // list view
    this->uiw->listView_2->setModel(this->m_tableProxyModel);
    this->uiw->listView_2->setRootIndex(this->m_tableProxyModel->mapFromSource(homeIndex));
    this->uiw->listView_2->setViewMode(QListView::IconMode);
    this->uiw->listView_2->setGridSize(QSize(80, 80));
    this->uiw->listView_2->setSelectionModel(this->uiw->tableView->selectionModel());
    
    /////
    QObject::connect(this->uiw->treeView, SIGNAL(clicked(const QModelIndex &)),
                     this, SLOT(slot_dir_tree_item_clicked(const QModelIndex &)));
    QObject::connect(this->uiw->tableView, SIGNAL(doubleClicked(const QModelIndex &)),
                     this, SLOT(slot_dir_file_view_double_clicked(const QModelIndex &)));
    QObject::connect(this->uiw->tableView, SIGNAL(drag_ready()),
                     this, SLOT(slot_drag_ready()));
    QObject::connect(this->uiw->listView_2, SIGNAL(doubleClicked(const QModelIndex &)),
                     this, SLOT(slot_dir_file_view_double_clicked(const QModelIndex &)));

    this->uiw->widget->onSetHome(this->user_home_path);

    //TODO 连接remoteview.treeView 的drag信号
    
    //显示SSH服务器信息
    QString ssh_server_version = libssh2_session_get_remote_version(this->conn->sess);
    int ssh_sftp_version = libssh2_sftp_get_version(this->ssh2_sftp);
    QString status_msg = QString("Ready. (%1  SFTP: V%2)").arg(ssh_server_version).arg(ssh_sftp_version); 
    this->status_bar->showMessage(status_msg);
}

void SFTPView::expand_to_home_directory(QModelIndex parent_model, int level)
{
    Q_ASSERT(!this->user_home_path.isEmpty());
    Q_UNUSED(parent_model);
    QString homePath = this->user_home_path;
    QStringList homePathParts = this->user_home_path.split('/');
    // qDebug()<<home_path_grade<<level<<row_cnt;
    QStringList stepPathParts;
    QString tmpPath;
    QModelIndex currIndex;
    QModelIndex proxyIndex;
    QModelIndex useIndex;

    // windows fix case: C:/abcd/efg/hi
    bool unixRootFix = true;
    if (homePath.length() > 1 && homePath.at(1) == ':') {
        unixRootFix = false;
    }
    
    for (int i = 0; i < homePathParts.count(); i++) {
        stepPathParts << homePathParts.at(i);
        tmpPath = (unixRootFix ? QString("/") : QString()) + stepPathParts.join("/");
        /// qDebug()<<tmpPath<<stepPathParts;
        currIndex = this->remote_dir_model->index(tmpPath);
        proxyIndex = this->m_treeProxyModel->mapFromSource(currIndex);
        useIndex = proxyIndex;
        this->uiw->treeView->expand(useIndex);
    }
    if (level == 1) {
        this->uiw->treeView->scrollTo(useIndex);
    }
    //qDebug()<<" root row count:"<< row_cnt ;
}

void SFTPView::expand_to_directory(QString path, int level)
{
    Q_ASSERT(!path.isEmpty());
    QString homePath = path;
    QStringList homePathParts = homePath.split('/');
    // qDebug()<<home_path_grade<<level<<row_cnt;
    QStringList stepPathParts;
    QString tmpPath;
    QModelIndex currIndex;
    QModelIndex proxyIndex;
    QModelIndex useIndex;

    // windows fix case: C:/abcd/efg/hi
    bool unixRootFix = true;
    if (homePath.length() > 1 && homePath.at(1) == ':') {
        unixRootFix = false;
    }
    
    for (int i = 0; i < homePathParts.count(); i++) {
        stepPathParts << homePathParts.at(i);
        tmpPath = (unixRootFix ? QString("/") : QString()) + stepPathParts.join("/");
        /// qDebug()<<tmpPath<<stepPathParts;
        currIndex = this->remote_dir_model->index(tmpPath);
        proxyIndex = this->m_treeProxyModel->mapFromSource(currIndex);
        useIndex = proxyIndex;
        this->uiw->treeView->expand(useIndex);
    }
    if (level == 1) {
        this->uiw->treeView->scrollTo(useIndex);
    }

}

void SFTPView::slot_disconnect_from_remote_host()
{
    this->uiw->treeView->setModel(0);
    // this->remote_dir_sort_filter_model->setSourceModel(0);
    // this->remote_dir_sort_filter_model_ex->setSourceModel(0);
    delete this->remote_dir_model;
    this->remote_dir_model = 0;
    // delete this->remote_dir_sort_filter_model_ex;
    // delete this->remote_dir_sort_filter_model;
}

void SFTPView::slot_backend_connection_disconnected()
{
    q_debug()<<"";
    // ask for close
    if (QMessageBox::question(this, tr("close this window?"),
                              tr("Connection lost, close session %1?").arg(this->windowTitle()),
                              QMessageBox::Ok|QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Ok) {
        this->close();
        // this->slot_close_session_window(false);
    }
}

void SFTPView::slot_dir_tree_customContextMenuRequested(const QPoint & pos)
{
    this->curr_item_view = static_cast<QAbstractItemView*>(sender());
    QPoint real_pos = this->curr_item_view->mapToGlobal(pos);
    real_pos = QPoint(real_pos.x() + 12, real_pos.y() + 36);
    attr_action->setEnabled(!this->in_remote_dir_retrive_loop);
    this->dir_tree_context_menu->popup(real_pos);
}

void SFTPView::slot_new_transfer()
{
    qDebug()<<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
     
    QString file_path;
    TaskPackage remote_pkg(PROTO_SFTP);
    
    if (this->in_remote_dir_retrive_loop) {
        QMessageBox::warning(this, tr("Notes:"), tr("Retriving remote directory tree,wait a minute please."));
        return ;
    }
    
    QItemSelectionModel *ism = this->curr_item_view->selectionModel();
    QModelIndex cidx, idx, pidx;

    if (ism == 0) {
        QMessageBox::critical(this, tr("Warning..."), tr("Maybe you haven't connected"));
        return;
    }
    if (!ism->hasSelection()) {
        return;
    }

    cidx = ism->currentIndex();
    pidx = cidx.parent();
    
    for (int i = ism->model()->rowCount(pidx) - 1; i >= 0; --i) {
        if (ism->isRowSelected(i, pidx)) {
            QModelIndex midx = ism->model()->index(i, 0, pidx);
            QModelIndex proxyIndex = midx;
            QModelIndex sourceIndex = (this->curr_item_view == this->uiw->treeView)
                ? this->m_treeProxyModel->mapToSource(midx)
                : this->m_tableProxyModel->mapToSource(midx);
            QModelIndex useIndex = sourceIndex;

            qDebug()<<this->remote_dir_model->fileName(useIndex)<<" "<<" "<<this->remote_dir_model->filePath(useIndex);
            file_path = this->remote_dir_model->filePath(useIndex);
            remote_pkg.files<<file_path;
        }
    }
    // QModelIndexList mil = ism->selectedIndexes();
    
    // for(int i = 0 ; i < mil.size(); i +=4 ) {
    //     QModelIndex midx = mil.at(i);
    //     QModelIndex proxyIndex = midx;
    //     QModelIndex sourceIndex = (this->curr_item_view == this->uiw->treeView)
    //         ? this->m_treeProxyModel->mapToSource(midx)
    //         : this->m_tableProxyModel->mapToSource(midx);
    //     QModelIndex useIndex = sourceIndex;

    //     qDebug()<<this->remote_dir_model->fileName(useIndex)<<" "<<" "<<this->remote_dir_model->filePath(useIndex);
    //     file_path = this->remote_dir_model->filePath(useIndex);
    //     remote_pkg.files<<file_path;
    // }

    remote_pkg.host = this->conn->hostName;
    remote_pkg.port = QString("%1").arg(this->conn->port);
    remote_pkg.username = this->conn->userName;
    remote_pkg.password = this->conn->password;
    remote_pkg.pubkey = this->conn->pubkey;

    this->slot_new_download_requested(remote_pkg);
}

QString SFTPView::get_selected_directory()
{
    QString file_path;
    
    QItemSelectionModel *ism = this->uiw->treeView->selectionModel();
    QModelIndex cidx, idx, pidx;
    
    if (ism == 0) {
        QMessageBox::critical(this, tr("Warning..."), tr("Maybe you haven't connected"));                
        return QString();
    }

    if (!ism->hasSelection()) {
        return QString();
    }

    cidx = ism->currentIndex();
    pidx = cidx.parent();
    
    for (int i = ism->model()->rowCount(pidx) - 1; i >= 0; --i) {
        if (ism->isRowSelected(i, pidx)) {
            QModelIndex midx = ism->model()->index(i, 0, pidx);
            QModelIndex proxyIndex = midx;
            QModelIndex sourceIndex = this->m_treeProxyModel->mapToSource(midx);
            QModelIndex useIndex = sourceIndex;

            if (this->remote_dir_model->isDir(useIndex)) {
                file_path = this->remote_dir_model->filePath(useIndex);
            } else {
                file_path = "";
            }
            break; // only use the first line
        }
    }
    
    // QModelIndexList mil = ism->selectedIndexes();

    // for (int i = 0 ; i < mil.size(); i += 4) {
    //     QModelIndex midx = mil.at(i);

    //     QModelIndex proxyIndex = midx;
    //     QModelIndex sourceIndex = this->m_treeProxyModel->mapToSource(midx);
    //     QModelIndex useIndex = sourceIndex;

    //     if (this->remote_dir_model->isDir(useIndex)) {
    //         file_path = this->remote_dir_model->filePath(useIndex);
    //     } else {
    //         file_path = "";
    //     }
    //     break; // only use the first line
    // }
    
    return file_path;
}

void SFTPView::set_user_home_path(const QString &user_home_path)
{
    this->user_home_path = user_home_path;
}

void SFTPView::setConnection(Connection *conn)
{
    Q_ASSERT(conn != NULL);
    Q_ASSERT(conn->isConnected() == true);

    this->conn = conn;
    QObject::connect(this->conn, SIGNAL(disconnected()), this, SLOT(slot_backend_connection_disconnected()));

    this->ssh2_sftp = libssh2_sftp_init(this->conn->sess);
    assert(this->ssh2_sftp != 0);    
    this->setWindowTitle(this->windowTitle() + ": " + this->conn->userName + "@" + this->conn->hostName);
}

void SFTPView::slot_close_session_window(bool ask)
{
    if (this->in_remote_dir_retrive_loop) {
        //TODO 怎么能友好的结束
        //QMessageBox::warning(this,tr("Attentions:"),tr("Retriving remote directory tree, wait a minute please.") );
        //return ;
        //如果说是在上传或者下载,则强烈建议用户先关闭传输窗口，再关闭连接
        if (this->own_progress_dialog != 0) {
            QMessageBox::warning(this, tr("Attentions:"), tr("You can't close connection when transfering file."));
            return;
        }
    }
    //this->setVisible(false);
    bool bok = false;
    if (ask) {
        if (QMessageBox::question(this, tr("Attemp to close this window?"),
                                  tr("Are you sure disconnect from %1?").arg(this->windowTitle()),
                                  QMessageBox::Ok|QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Ok) {
            bok = true;
        }
    }

    if ((ask && bok == true) || ask == false) {
        this->setVisible(false);
        q_debug()<<"delete remote view";
        Q_ASSERT(this->parentWidget() != NULL);
        this->main_mdi_area->removeSubWindow(this->parentWidget());
        delete this;
    }
}

void SFTPView::closeEvent(QCloseEvent *event)
{
    qDebug()<<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    event->ignore();

    this->slot_close_session_window(this->conn->isConnected());
}
void SFTPView::slot_custom_ui_area()
{
    qDebug()<<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    
    QSizePolicy sp;
    sp.setVerticalPolicy(QSizePolicy::Ignored);
    this->uiw->listView->setSizePolicy(sp) ;
    //这个设置必须在show之前设置才有效果
    this->uiw->splitter->setStretchFactor(0,1);
    this->uiw->splitter->setStretchFactor(1,2);

    this->uiw->splitter_2->setStretchFactor(0,6);
    this->uiw->splitter_2->setStretchFactor(1,1);
    // this->uiw->listView->setVisible(false);// 暂时没有功能在里面先隐藏掉
    //this->uiw->tableView->setVisible(false);
    q_debug()<<this->geometry();
    this->setGeometry(this->x(), this->y(), this->width(), this->height()*2);
    // this->resize(this->width(), this->height() + 200);
    q_debug()<<this->geometry();
}

void SFTPView::slot_enter_remote_dir_retrive_loop()
{
    qDebug()<<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    this->in_remote_dir_retrive_loop = true ;
    this->remote_dir_model->set_keep_alive(false);
    this->orginal_cursor = this->uiw->splitter->cursor();
    this->uiw->splitter->setCursor(Qt::BusyCursor);
}

void SFTPView::slot_leave_remote_dir_retrive_loop()
{
    qDebug()<<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;

    this->uiw->splitter->setCursor(this->orginal_cursor);
    this->remote_dir_model->set_keep_alive(true);
    this->in_remote_dir_retrive_loop = false ;
    for (int i = 0 ; i < this->m_tableProxyModel->rowCount(this->uiw->tableView->rootIndex()); i ++) {
        this->uiw->tableView->setRowHeight(i, this->table_row_height);
    }
    this->uiw->tableView->resizeColumnToContents(0);
    this->onUpdateEntriesStatus();
}

void SFTPView::update_layout()
{
    qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    
    QString file_path ;
    
    QItemSelectionModel *ism = this->uiw->treeView->selectionModel();
    QModelIndex cidx, idx, pidx;

    if (ism == 0) {
        //QMessageBox::critical(this,tr("waring..."),tr("maybe you haven't connected"));                
        //return file_path ;
        qDebug()<<"why???? no QItemSelectionModel??";   
        return;
    }
    
    if (!ism->hasSelection()) {
        qDebug()<<"selectedIndexes count :"<<ism->hasSelection()<<"why no item selected????";
        return;
    }
    
    // QModelIndexList mil = ism->selectedIndexes();
    
    // if (mil.count() == 0) {
    //     qDebug()<<"selectedIndexes count :"<<mil.count()<<"why no item selected????";
    // }

    QModelIndex proxyIndex;
    QModelIndex sourceIndex;
    QModelIndex useIndex;

    cidx = ism->currentIndex();
    pidx = cidx.parent();
    
    for (int i = ism->model()->rowCount(pidx) - 1; i >= 0; --i) {
        if (ism->isRowSelected(i, pidx)) {
            QModelIndex midx = ism->model()->index(i, 0, pidx);
            proxyIndex = midx;
            sourceIndex = this->m_treeProxyModel->mapToSource(proxyIndex);
            useIndex = sourceIndex;

            NetDirNode *dti = static_cast<NetDirNode*>(useIndex.internalPointer());
            // (this->remote_dir_sort_filter_model_ex->mapToSource(midx).internalPointer());
            qDebug()<<dti->_fileName<<" "<< dti->fullPath;
            file_path = dti->fullPath;
            dti->retrFlag = POP_NO_NEED_WITH_DATA; // 1;
            dti->prevFlag = POP_NEWEST; // 9;
            // this->remote_dir_model->slot_remote_dir_node_clicked(this->remote_dir_sort_filter_model_ex->mapToSource(midx));
            this->remote_dir_model->slot_remote_dir_node_clicked(useIndex);
        }
    }
    
    // for (int i = 0 ; i < mil.size(); i += 4) {
    //     QModelIndex midx = mil.at(i);
    //     qDebug()<<midx;

    //     proxyIndex = midx;
    //     sourceIndex = this->m_treeProxyModel->mapToSource(proxyIndex);
    //     useIndex = sourceIndex;

    //     NetDirNode *dti = static_cast<NetDirNode*>(useIndex.internalPointer());
    //     qDebug()<<dti->_fileName<<" "<< dti->fullPath;
    //     file_path = dti->fullPath;
    //     dti->retrFlag = POP_NO_NEED_WITH_DATA; // 1;
    //     dti->prevFlag = POP_NEWEST; // 9;
    //     this->remote_dir_model->slot_remote_dir_node_clicked(useIndex);
    // }
}

// inhirented from base class
// void SFTPView::slot_refresh_directory_tree()
// {
//     this->update_layout();
// }

void SFTPView::slot_show_properties()
{
    QItemSelectionModel *ism = this->curr_item_view->selectionModel();
    QModelIndex cidx, idx, pidx;

    if (ism == 0) {
        qDebug()<<"why???? no QItemSelectionModel??";        
        return;
    }

    if (!ism->hasSelection()) {
        return;
    }
    
    cidx = ism->currentIndex();
    pidx = cidx.parent();
    idx = ism->model()->index(cidx.row(), 0, cidx.parent());

    QModelIndexList aim_mil;
    for (int i = 0; i < ism->model()->columnCount(pidx); i++) {
        idx = ism->model()->index(cidx.row(), i, pidx);
        if (this->curr_item_view == this->uiw->treeView) {
            idx = this->m_treeProxyModel->mapToSource(idx);
        } else if (this->curr_item_view == this->uiw->listView_2) {
            QModelIndex currIndex;
            QModelIndex tmpIndex;

            currIndex = this->m_tableProxyModel->mapToSource(idx);
            aim_mil<<currIndex;
            // aim_mil<<mil.at(i);
            for (int col = 1; col < 4; col ++) {
                tmpIndex = this->remote_dir_model->index(currIndex.row(), col, currIndex.parent());
                aim_mil << tmpIndex;
            }

        } else if (this->curr_item_view == this->uiw->tableView) {
            idx = this->m_tableProxyModel->mapToSource(idx);
        }
        aim_mil<<idx;
    }
    // TODO should be fix win x64
    // QModelIndexList mil = ism->selectedIndexes();
    // QModelIndexList aim_mil;
    // if (this->curr_item_view == this->uiw->treeView) {
    //     for (int i = 0 ; i < mil.count() ; i ++) {
    //         aim_mil<<this->m_treeProxyModel->mapToSource(mil.at(i));
    //         // aim_mil << mil.at(i);
    //     }
    // } else if (this->curr_item_view == this->uiw->listView_2) {
    //     QModelIndex currIndex;
    //     QModelIndex tmpIndex;
    //     for (int i = 0 ; i < mil.count() ; i ++) {
    //         currIndex = this->m_tableProxyModel->mapToSource(mil.at(i));
    //         aim_mil<<currIndex;
    //         // aim_mil<<mil.at(i);
    //         for (int col = 1; col < 4; col ++) {
    //             tmpIndex = this->remote_dir_model->index(currIndex.row(), col, currIndex.parent());
    //             aim_mil << tmpIndex;
    //         }
    //     }
    // } else if (this->curr_item_view == this->uiw->tableView) {
    //     if (mil.count() % 4 == 0) {
    //         for (int i = 0 ; i < mil.count() ; i ++) {
    //             aim_mil<<this->m_tableProxyModel->mapToSource(mil.at(i));
    //             // aim_mil<<mil.at(i);
    //         }
    //     } else {
    //         QModelIndex currIndex;
    //         QModelIndex tmpIndex;
    //         for (int i = 0 ; i < mil.count() ; i ++) {
    //             currIndex = this->m_tableProxyModel->mapToSource(mil.at(i));
    //             aim_mil<<currIndex;
    //             // aim_mil<<mil.at(i);
    //             for (int col = 1; col < 4; col ++) {
    //                 tmpIndex = this->remote_dir_model->index(currIndex.row(), col, currIndex.parent());
    //                 aim_mil << tmpIndex;
    //             }
    //         }
    //     }
    // } else {
    //     Q_ASSERT(1 == 2);
    // }
    if (aim_mil.count() == 0) {
        qDebug()<<"why???? no QItemSelectionModel??";
        return;
    }
    //  文件类型，大小，几个时间，文件权限
    //TODO 从模型中取到这些数据并显示在属性对话框中。
    FileProperties *fp = new FileProperties(this);
    fp->set_ssh2_sftp(this->ssh2_sftp);
    fp->setConnection(this->conn);
    fp->set_file_info_model_list(aim_mil);
    if (fp->exec() == QDialog::Accepted) {
    }
    delete fp;
}

void SFTPView::slot_mkdir()
{
    QString dir_name;
    
    QItemSelectionModel *ism = this->curr_item_view->selectionModel();
    QModelIndex cidx, idx, pidx;
    
    if (ism == 0) {
        qDebug()<<"why???? no QItemSelectionModel??";
        QMessageBox::critical(this, tr("Warning..."), tr("Maybe you haven't connected"));                
        return;
    }

    if (!ism->hasSelection()) {
        qDebug()<<"selectedIndexes count :"<<ism->hasSelection()<<"why no item selected????";
        QMessageBox::critical(this, tr("Warning..."), tr("No item selected"));
        return;
    }
    
    cidx = ism->currentIndex();
    idx = ism->model()->index(cidx.row(), 0, cidx.parent());

    // QModelIndexList mil = ism->selectedIndexes();
    
    // if (mil.count() == 0) {
    //     qDebug()<<"selectedIndexes count :"<<mil.count()<<"why no item selected????";
    //     QMessageBox::critical(this, tr("Warning..."), tr("No item selected"));
    //     return;
    // }
    
    // QModelIndex midx = mil.at(0);
    QModelIndex midx = idx;
    QModelIndex proxyIndex = midx;
    QModelIndex sourceIndex = (this->curr_item_view == this->uiw->treeView)
        ? this->m_treeProxyModel->mapToSource(midx)
        : this->m_tableProxyModel->mapToSource(midx);
    QModelIndex useIndex = sourceIndex;
    NetDirNode *dti = (NetDirNode*)(useIndex.internalPointer());
    
    //检查所选择的项是不是目录
    if (!this->remote_dir_model->isDir(useIndex)) {
        QMessageBox::critical(this, tr("waring..."), tr("The selected item is not a directory."));
        return;
    }
    
    dir_name = QInputDialog::getText(this, tr("Create directory:"),
                                     tr("Input directory name:").leftJustified(60, ' '),
                                     QLineEdit::Normal,
                                     tr("new_direcotry"));
    if (dir_name == QString::null) {
        return; // user canceled
    } 
    if (dir_name.length() == 0) {
        qDebug()<<"selectedIndexes count :"<<ism->hasSelection()<<"why no item selected????";
        QMessageBox::critical(this, tr("waring..."), tr("no directory name supplyed "));
        return;
    }
    //TODO 将 file_path 转换编码再执行下面的操作
    QPersistentModelIndex *persisIndex = new QPersistentModelIndex(useIndex);
    q_debug()<<persisIndex->parent();
    this->remote_dir_model->slot_execute_command(dti, persisIndex, SSH2_FXP_MKDIR, dir_name);
    
}

// TODO when remove multi files, the view's behaviour is strange
// should fixed now.
void SFTPView::rm_file_or_directory_recursively()
{
    qDebug()<<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    
    QItemSelectionModel *ism = this->curr_item_view->selectionModel();
    QModelIndex cidx, idx, pidx;
    
    if (ism == 0) {
        qDebug()<<"why???? no QItemSelectionModel??";
        QMessageBox::critical(this, tr("Warning..."), tr("Maybe you haven't connected"));                
        return;
    }
    
    // QModelIndexList mil = ism->selectedIndexes();
    
    // if (mil.count() == 0) {
    //     qDebug()<<"selectedIndexes count:"<<mil.count()<<"why no item selected????";
    //     QMessageBox::critical(this, tr("Warning..."), tr("No item selected"));
    //     return;
    // }

    if (!ism->hasSelection()) {
        qDebug()<<"selectedIndexes count:"<<ism->hasSelection()<<"why no item selected????";
        QMessageBox::critical(this, tr("Warning..."), tr("No item selected"));
        return;
    }

    cidx = ism->currentIndex();
    pidx = cidx.parent();

    bool firstWarning = true;
    for (int i = ism->model()->rowCount(pidx) - 1; i >= 0; --i) {
        if (!ism->isRowSelected(i, pidx)) {
            continue;
        }
        QModelIndex midx = ism->model()->index(i, 0, pidx);
        QModelIndex proxyIndex = midx;
        QModelIndex sourceIndex = (this->curr_item_view == this->uiw->treeView)
            ? this->m_treeProxyModel->mapToSource(midx)
            : this->m_tableProxyModel->mapToSource(midx);
        QModelIndex useIndex = sourceIndex;
        QModelIndex parent_model =  useIndex.parent();
        NetDirNode *parent_item = static_cast<NetDirNode*>(parent_model.internalPointer());

        if (firstWarning) { // 只有第一次提示用户操作，其他的不管
            QString hintMsg;
            if (ism->model()->rowCount(pidx) > 0) {
                // select multi lines/files
                hintMsg = QString(tr("Are you sure remove all of these files/directories?"));
            } else {
                hintMsg = this->remote_dir_model->isDir(useIndex) 
                    ? QString(tr("Are you sure remove this directory and it's subnodes?\n    %1/"))
                    .arg(this->remote_dir_model->filePath(useIndex))
                    : QString(tr("Are you sure remove this file?\n    %1"))
                    .arg(this->remote_dir_model->filePath(useIndex));
            }
            if (QMessageBox::warning(this, tr("Warning:"), hintMsg,
                                     QMessageBox::Yes, QMessageBox::Cancel) == QMessageBox::Yes) {
            } else {
                break;
            }
            firstWarning = false;
        }

        QPersistentModelIndex *persisIndex = new QPersistentModelIndex(parent_model);
        this->remote_dir_model->slot_execute_command(parent_item, persisIndex,
                                                     SSH2_FXP_REMOVE, this->remote_dir_model->fileName(useIndex));
    }
    

    // TODO magic number
    // bool firstWarning = true;
    // for (int i = mil.count() - 1 - 3; i >= 0; i -= 4) {
    //     QModelIndex midx = mil.at(i);
    //     QModelIndex proxyIndex = midx;
    //     QModelIndex sourceIndex = (this->curr_item_view == this->uiw->treeView)
    //         ? this->m_treeProxyModel->mapToSource(midx)
    //         : this->m_tableProxyModel->mapToSource(midx);
    //     QModelIndex useIndex = sourceIndex;
    //     QModelIndex parent_model =  useIndex.parent();
    //     NetDirNode *parent_item = static_cast<NetDirNode*>(parent_model.internalPointer());

    //     if (firstWarning) { // 只有第一次提示用户操作，其他的不管
    //         QString hintMsg;
    //         if (mil.count()/4 > 1) {
    //             // select multi lines/files
    //             hintMsg = QString(tr("Are you sure remove all of these files/directories?"));
    //         } else {
    //             hintMsg = this->remote_dir_model->isDir(useIndex) 
    //                 ? QString(tr("Are you sure remove this directory and it's subnodes?\n    %1/"))
    //                 .arg(this->remote_dir_model->filePath(useIndex))
    //                 : QString(tr("Are you sure remove this file?\n    %1"))
    //                 .arg(this->remote_dir_model->filePath(useIndex));
    //         }
    //         if (QMessageBox::warning(this, tr("Warning:"), hintMsg,
    //                                  QMessageBox::Yes, QMessageBox::Cancel) == QMessageBox::Yes) {
    //         } else {
    //             break;
    //         }
    //         firstWarning = false;
    //     }

    //     QPersistentModelIndex *persisIndex = new QPersistentModelIndex(parent_model);
    //     this->remote_dir_model->slot_execute_command(parent_item, persisIndex,
    //                                                  SSH2_FXP_REMOVE, this->remote_dir_model->fileName(useIndex));
    // }
}

void SFTPView::slot_rename()
{
    // qDebug()<<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    // q_debug()<<this->uiw->tableView->hasFocus()<<this->uiw->treeView->hasFocus();
    QAbstractItemView *curr_view = this->uiw->treeView->hasFocus() ? (QAbstractItemView*)this->uiw->treeView
        : (this->uiw->tableView->hasFocus() ? (QAbstractItemView*)this->uiw->tableView : NULL);
    if (curr_view == NULL) {
        q_debug()<<"no focus view.";
        return;
    }
    QItemSelectionModel *ism = curr_view->selectionModel();
    // QItemSelectionModel *ism = this->curr_item_view->selectionModel();
    QModelIndex cidx, idx;
    
    if (ism == 0) {
        qDebug()<<"why???? no QItemSelectionModel??";
        QMessageBox::critical(this, tr("waring..."), tr("maybe you haven't connected"));                
        return;
    }
    
    // QModelIndexList mil = ism->selectedIndexes();
    
    // if (mil.count() == 0 ) {
    //     qDebug()<<"selectedIndexes count :"<<mil.count()<<" why no item selected????";
    //     QMessageBox::critical(this, tr("waring..."), tr("no item selected"));
    //     return;
    // }

    if (!ism->hasSelection()) {
        qDebug()<<"selectedIndexes count :"<<ism->hasSelection()<<" why no item selected????";
        QMessageBox::critical(this, tr("waring..."), tr("no item selected"));
        return;
    }
    cidx = ism->currentIndex();
    idx = ism->model()->index(cidx.row(), 0, cidx.parent());
    // this->curr_item_view->edit(idx);
    curr_view->edit(idx);
}
void SFTPView::slot_copy_path()
{
    QItemSelectionModel *ism = this->curr_item_view->selectionModel();
    QModelIndex cidx, idx;
    
    if (ism == 0) {
        qDebug()<<"why???? no QItemSelectionModel??";
        QMessageBox::critical(this, tr("Warning..."), tr("Maybe you haven't connected"));
        return;
    }
    
    // QModelIndexList mil = ism->selectedIndexes();
    // if (mil.count() == 0) {
    //     qDebug()<<"selectedIndexes count :"<< mil.count() << " why no item selected????";
    //     QMessageBox::critical(this, tr("Warning..."), tr("No item selected").leftJustified(50, ' '));
    //     return;
    // }
    if (!ism->hasSelection()) {
        qDebug()<<"selectedIndexes count :"<< ism->hasSelection() << " why no item selected????";
        QMessageBox::critical(this, tr("Warning..."), tr("No item selected").leftJustified(50, ' '));
        return;
    }
    
    cidx = ism->currentIndex();
    idx = ism->model()->index(cidx.row(), 0, cidx.parent());

    QModelIndex midx = idx;
    QModelIndex proxyIndex = midx;
    QModelIndex sourceIndex = (this->curr_item_view == this->uiw->treeView)
        ? this->m_treeProxyModel->mapToSource(midx)
        : this->m_tableProxyModel->mapToSource(midx);
    QModelIndex useIndex = sourceIndex;

    QApplication::clipboard()->setText(this->remote_dir_model->filePath(useIndex));
}

void SFTPView::slot_copy_url()
{
    QItemSelectionModel *ism = this->curr_item_view->selectionModel();
    QModelIndex cidx, idx;

    if (ism == 0) {
        qDebug()<<"why???? no QItemSelectionModel??";
        QMessageBox::critical(this, tr("Warning..."), tr("Maybe you haven't connected"));
        return;
    }
    
    // QModelIndexList mil = ism->selectedIndexes();
    // if (mil.count() == 0) {
    //     qDebug()<<" selectedIndexes count :"<< mil.count() << " why no item selected????";
    //     QMessageBox::critical(this, tr("Warning..."), tr("No item selected").leftJustified(50, ' '));
    //     return;
    // }
    if (!ism->hasSelection()) {
        qDebug()<<" selectedIndexes count :"<< ism->hasSelection() << " why no item selected????";
        QMessageBox::critical(this, tr("Warning..."), tr("No item selected").leftJustified(50, ' '));
        return;
    }

    cidx = ism->currentIndex();
    idx = ism->model()->index(cidx.row(), 0, cidx.parent());
    
    QModelIndex midx = idx;
    QModelIndex proxyIndex = midx;
    QModelIndex sourceIndex = (this->curr_item_view == this->uiw->treeView)
        ? this->m_treeProxyModel->mapToSource(midx)
        : this->m_tableProxyModel->mapToSource(midx);
    QModelIndex useIndex = sourceIndex;

    
    QString url = QString("sftp://%1@%2:%3%4").arg(this->conn->userName)
        .arg(this->conn->hostName).arg(this->conn->port)
        .arg(this->remote_dir_model->filePath(useIndex));
    QApplication::clipboard()->setText(url);
}


void SFTPView::slot_new_upload_requested(TaskPackage local_pkg, TaskPackage remote_pkg)
{
    SFTPView *remote_view = this;
    ProgressDialog *pdlg = new ProgressDialog();

    pdlg->set_transfer_info(local_pkg, remote_pkg);

    QObject::connect(pdlg, SIGNAL(transfer_finished(int, QString)),
                     remote_view, SLOT(slot_transfer_finished (int, QString)));

    this->main_mdi_area->addSubWindow(pdlg);
    pdlg->show();
    this->own_progress_dialog = pdlg;

}
void SFTPView::slot_new_upload_requested(TaskPackage local_pkg)
{
    QString remote__fileName ;
    SFTPView *remote_view = this ;
    TaskPackage remote_pkg(PROTO_SFTP);

    qDebug()<<" window title :" << remote_view->windowTitle();

    remote__fileName = remote_view->get_selected_directory();    

    if (remote__fileName.length() == 0) {
        QMessageBox::critical(this, tr("Warning..."), tr("you should selecte a remote file directory."));
    } else {
        remote_pkg.files<<remote__fileName;

        remote_pkg.host = this->conn->hostName;
        remote_pkg.port = QString("%1").arg(this->conn->port);
        remote_pkg.username = this->conn->userName;
        remote_pkg.password = this->conn->password;
        remote_pkg.pubkey = this->conn->pubkey;

        this->slot_new_upload_requested(local_pkg, remote_pkg);
    }
}

void SFTPView::slot_new_download_requested(TaskPackage local_pkg, TaskPackage remote_pkg)
{
    qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    SFTPView *remote_view = this ;
        
    ProgressDialog *pdlg = new ProgressDialog(0);
    // src is remote file , dest if localfile 
    pdlg->set_transfer_info(remote_pkg, local_pkg);
    QObject::connect(pdlg, SIGNAL(transfer_finished(int, QString)),
                     this, SLOT(slot_transfer_finished(int, QString)));
    this->main_mdi_area->addSubWindow(pdlg);
    pdlg->show();
    this->own_progress_dialog = pdlg;
}
void SFTPView::slot_new_download_requested(TaskPackage remote_pkg) 
{
    qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    
    QString local_file_path;
    SFTPView *remote_view = this;
    TaskPackage local_pkg(PROTO_FILE);
    
    local_file_path = this->local_view->get_selected_directory();
    
    qDebug()<<local_file_path;
    if (local_file_path.length() == 0 
        || !QFileInfo(local_file_path).isDir()) {
        //        || !is_dir(GlobalOption::instance()->locale_codec->fromUnicode(local_file_path).data())) {
        qDebug()<<" selected a local file directory  please";
        QMessageBox::critical(this, tr("waring..."), tr("you should selecte a local file directory."));
    } else {
        local_pkg.files<<local_file_path;
        this->slot_new_download_requested(local_pkg, remote_pkg);
    }
}

void SFTPView::slot_transfer_finished(int status, QString errorString)
{
    qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__<<status; 
    SFTPView *remote_view = this;
    
    ProgressDialog *pdlg = (ProgressDialog*)sender();

    this->main_mdi_area->removeSubWindow(pdlg->parentWidget());

    delete pdlg;
    this->own_progress_dialog = 0;

    if (status == 0 // || status == 3
        ) {
        //TODO 通知UI更新目录结构,在某些情况下会导致左侧树目录变空。
        //int transfer_type = pdlg->get_transfer_type();
        //if ( transfer_type == TransferThread::TRANSFER_GET )
        {
            this->local_view->update_layout();
        }
        //else if ( transfer_type == TransferThread::TRANSFER_PUT )
        {
            remote_view->update_layout();
        }
        //else
        {
            // xxxxx: 没有预期到的错误
            //assert ( 1== 2 );
        }
    } else if (status == 52 // Transportor::ERROR_CANCEL
               ) {
        // user cancel, show nothing
    } else if (status != 0 // && status != 3
               ) {
        QString errmsg = QString(errorString + " Code: %1").arg(status);
        if (errmsg.length() < 50) errmsg = errmsg.leftJustified(50);
        QMessageBox::critical(this, QString(tr("Error: ")), errmsg);
    } else {
        Q_ASSERT(1 == 2);
    }
    // remote_view->slot_leave_remote_dir_retrive_loop();
}

/**
 *
 * index 是proxy的index 
 */
void SFTPView::slot_dir_tree_item_clicked(const QModelIndex & index)
{
    qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__<<index<<index.row();
    QString file_path;
    QModelIndex proxyIndex;
    QModelIndex sourceIndex;
    QModelIndex useIndex;

    proxyIndex = index;
    sourceIndex = this->m_treeProxyModel->mapToSource(index);
    // sourceIndex = index;
    useIndex = sourceIndex;

    // NetDirNode *pnode = static_cast<NetDirNode*>(useIndex.internalPointer());
    // NetDirNode *node = NULL;
    // this->remote_dir_model->dump_tree_node_item(pnode);

    // qDebug()<<"root childs:"<<this->remote_dir_model->rowCount(useIndex);
    // node = static_cast<NetDirNode*>(this->remote_dir_model->parent(useIndex).internalPointer());
    // if (node == NULL) {
    //     qDebug()<<"this is the top root node";
    // } else {
    //     this->remote_dir_model->dump_tree_node_item(node);
    // }
    
    this->remote_dir_model->slot_remote_dir_node_clicked(useIndex);

    this->uiw->tableView->setRootIndex(this->m_tableProxyModel->mapFromSource(useIndex));
    for (int i = 0; i < this->remote_dir_model->rowCount(useIndex); ++i) {
        this->uiw->tableView->setRowHeight(i, this->table_row_height);
    }

    // file_path = this->remote_dir_model->filePath(useIndex);
    // this->uiw->tableView->setRootIndex(this->remote_dir_model->index(file_path));
    // for (int i = 0 ; i < this->remote_dir_model->rowCount(this->remote_dir_model->index(file_path)); i ++ ) {
    //     this->uiw->tableView->setRowHeight(i, this->table_row_height);
    // }
    this->uiw->tableView->resizeColumnToContents(0);

    // useIndex = this->uiw->tableView->rootIndex();
    // qDebug()<<useIndex<<this->remote_dir_model->rowCount(useIndex);
    // node = static_cast<NetDirNode*>(useIndex.internalPointer());
    // this->remote_dir_model->dump_tree_node_item(pnode);

    this->uiw->listView_2->setRootIndex(this->m_tableProxyModel->mapFromSource(useIndex));

    if (this->remote_dir_model->filePath(useIndex) == ""
        && this->remote_dir_model->fileName(useIndex) == "/") {
        this->uiw->widget->onNavToPath(this->remote_dir_model->fileName(useIndex));
    } else {
        this->uiw->widget->onNavToPath(this->remote_dir_model->filePath(useIndex));
    }
    this->onUpdateEntriesStatus();
}

void SFTPView::slot_dir_file_view_double_clicked(const QModelIndex & index)
{
    qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    // TODO if the clicked item is direcotry ,
    // expand left tree dir and update right table view
    // got the file path , tell tree ' model , then expand it
    // 文件列表中的双击事件
    //1。本地主机，如果是目录，则打开这个目录，如果是文件，则使用本机的程序打开这个文件
    //2。对于远程主机，　如果是目录，则打开这个目录，如果是文件，则提示是否要下载它(或者也可以直接打开这个文件）。

    QModelIndex proxyIndex = index;
    QModelIndex sourceIndex = this->m_tableProxyModel->mapToSource(proxyIndex);
    QModelIndex useIndex = sourceIndex;
    QModelIndex idx, idx2, idx_top_left, idx_bottom_right;

    QString file_path;
    if (this->remote_dir_model->isDir(useIndex) || this->remote_dir_model->isSymLinkToDir(useIndex)) {
        this->uiw->treeView->expand(this->m_treeProxyModel->mapFromSource(useIndex).parent());
        this->uiw->treeView->expand(this->m_treeProxyModel->mapFromSource(useIndex));
        this->slot_dir_tree_item_clicked(this->m_treeProxyModel->mapFromSource(useIndex));

        // this->uiw->treeView->selectionModel()->clearSelection();
        // this->uiw->treeView->selectionModel()->select(this->m_treeProxyModel->mapFromSource(useIndex), 
        //                                                     QItemSelectionModel::Select);
        
        idx = useIndex;
        idx2 = idx.parent();

        idx_top_left = this->remote_dir_model->index(idx.row(), 0, idx2);
        idx_bottom_right = this->remote_dir_model->index(idx.row(), 
                                                         this->remote_dir_model->columnCount(idx)-1,
                                                         idx2);
        QItemSelection iselect(this->m_treeProxyModel->mapFromSource(idx_top_left),
                               this->m_treeProxyModel->mapFromSource(idx_bottom_right));
        this->uiw->treeView->selectionModel()->clearSelection();
        this->uiw->treeView->selectionModel()->select(iselect, QItemSelectionModel::Select);
        this->uiw->treeView->selectionModel()->setCurrentIndex(this->m_treeProxyModel->mapFromSource(idx_top_left),
                                                               QItemSelectionModel::Select);
        this->uiw->treeView->setFocus(Qt::ActiveWindowFocusReason);
    } else if (this->remote_dir_model->isSymLink(useIndex)) {
        NetDirNode *node_item = (NetDirNode*)useIndex.internalPointer();
        QPersistentModelIndex *persisIndex = new QPersistentModelIndex(useIndex);
        q_debug()<<node_item->fullPath;
        this->remote_dir_model->dump_tree_node_item(node_item);
        this->remote_dir_model->slot_execute_command(node_item, persisIndex,
                                                     SSH2_FXP_REALPATH, QString(""));
    } else {
        q_debug()<<"double clicked a regular file, no op now, only now";
    }
}

void SFTPView::slot_drag_ready()
{
    //qDebug() <<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    //TODO 处理从树目录及文件列表视图中的情况
    //QAbstractItemView * sender_view = qobject_cast<QAbstractItemView*>(sender());
    QString  temp_file_path, remote__fileName;
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    
    //这个视图中所选择的目录优先，如果没有则查找左侧目录树是是否有选择的目录，如果再找不到，则使用右侧表视图的根
    QItemSelectionModel *ism = this->uiw->tableView->selectionModel();
    QModelIndex cidx, idx, pidx;

    // QModelIndexList mil = ism->selectedIndexes();
    // if (mil.count() == 0) {
    //     ism = this->uiw->treeView->selectionModel();
    //     mil = ism->selectedIndexes();
    // }
    if (!ism->hasSelection()) {
        ism = this->uiw->treeView->selectionModel();
    }

    TaskPackage tpkg(PROTO_SFTP);

    tpkg.host = this->conn->hostName;
    tpkg.port = QString("%1").arg(this->conn->port);
    tpkg.username = this->conn->userName;
    tpkg.password = this->conn->password;
    tpkg.pubkey = this->conn->pubkey;

    cidx = ism->currentIndex();
    pidx = cidx.parent();
    // qDebug()<<cidx<<pidx<<ism->model()->rowCount(pidx);

    for (int i = ism->model()->rowCount(pidx) - 1 ; i >= 0 ; --i) {
        if (ism->isRowSelected(i, pidx)) {
            QModelIndex midx = ism->model()->index(i, 0, pidx);
            if (ism->model() == this->m_treeProxyModel) {
                temp_file_path = this->remote_dir_model
                    ->filePath(this->m_treeProxyModel->mapToSource(midx));
                // qDebug()<<"tree";
            } else if (ism->model() == this->m_tableProxyModel) {
                temp_file_path = this->remote_dir_model
                    ->filePath(this->m_tableProxyModel->mapToSource(midx));
                // qDebug()<<"table";
            } else if (ism->model() == this->remote_dir_model) {
                temp_file_path = this->remote_dir_model->filePath(midx);
            } else {
                // qDebug()<<"not possible";
                Q_ASSERT(1==2);
            }
            // temp_file_path = (qobject_cast<RemoteDirModel*>(this->remote_dir_model))
            //    ->filePath(midx);

            tpkg.files<<temp_file_path;
            // qDebug()<<"Drag Row selected:"<<i<<temp_file_path;
        } else {
            // qDebug()<<"Row not selected:"<<i;
        }
    }

    // qDebug()<<tpkg;
    
    mimeData->setData("application/task-package", tpkg.toRawData());
    drag->setMimeData(mimeData);
    
    if (ism->hasSelection()) {
        Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction);
        Q_UNUSED(dropAction);
    }
    qDebug()<<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__<<"drag->exec returned"<<mimeData->formats();
}

bool SFTPView::slot_drop_mime_data(const QMimeData *data, Qt::DropAction action,
                                     int row, int column, const QModelIndex &parent)
{
    qDebug()<<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
    Q_UNUSED(row);
    Q_UNUSED(column);
    
    TaskPackage local_pkg(PROTO_FILE);
    TaskPackage remote_pkg(PROTO_SFTP);
   
    NetDirNode *aim_item = static_cast<NetDirNode*>(parent.internalPointer());        
    QString remote_file_name = aim_item->fullPath;

    remote_pkg.files<<remote_file_name;

    remote_pkg.host = this->conn->hostName;
    remote_pkg.port = QString("%1").arg(this->conn->port);
    remote_pkg.username = this->conn->userName;
    remote_pkg.password = this->conn->password;
    remote_pkg.pubkey = this->conn->pubkey;

    if (data->hasFormat("application/task-package")) {
        local_pkg = TaskPackage::fromRawData(data->data("application/task-package"));
        qDebug()<<local_pkg;
        if (local_pkg.isValid(local_pkg)) {
            // fixed 两个sftp服务器间拖放的情况, 也可以，已经完成
            this->slot_new_upload_requested(local_pkg, remote_pkg);
        }
    } else if (data->hasFormat("text/uri-list")) {
        // from localview
        QList<QUrl> files = data->urls();
        if (files.count() == 0) {
            // return false;
            assert(0);
        } else {
            for (int i = 0 ; i < files.count(); i++) {
                QString path = files.at(i).path();
                #ifdef WIN32
                // because on win32, now path=/C:/xxxxx
                path = path.right(path.length() - 1);
                #endif
                local_pkg.files<<path;
            }
            this->slot_new_upload_requested(local_pkg, remote_pkg);
        }        
    } else {
        qDebug()<<"invalid mime type:"<<data->formats();
    }
    qDebug()<<"drop mime data processed";
    
    return true;
}

void SFTPView::slot_show_hidden(bool show)
{
    DirTreeSortFilterModel *tree = (DirTreeSortFilterModel*)this->m_treeProxyModel;
    DirTableSortFilterModel *table = (DirTableSortFilterModel*)this->m_tableProxyModel;
    if (show) {
        tree->setFilter(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot);
        table->setFilter(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot);
    } else {
        tree->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
        table->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    }
    this->onUpdateEntriesStatus();
}

void SFTPView::onUpdateEntriesStatus()
{
    QModelIndex proxyIndex = this->uiw->tableView->rootIndex();
    QModelIndex sourceIndex = this->m_tableProxyModel->mapToSource(proxyIndex);
    QModelIndex useIndex = sourceIndex;
    int entries = this->m_tableProxyModel->rowCount(proxyIndex);
    QString msg = QString("%1 entries").arg(entries);
    this->entriesLabel->setText(msg);
}

void SFTPView::onDirectoryLoaded(const QString &path)
{
    q_debug()<<path;
    // if (this->model->filePath(this->uiw->tableView->rootIndex()) == path) {
    //     this->uiw->tableView->resizeColumnToContents(0);
    //     this->onUpdateEntriesStatus();
    // }

    if (this->is_dir_complete_request) {
        this->is_dir_complete_request = false;

        QString prefix = this->dir_complete_request_prefix;
        
        QStringList matches;
        QModelIndex currIndex;
        QModelIndex sourceIndex = this->remote_dir_model->index(path);
        int rc = this->remote_dir_model->rowCount(sourceIndex);
        q_debug()<<"lazy load dir rows:"<<rc;
        for (int i = rc - 1; i >= 0; --i) {
            currIndex = this->remote_dir_model->index(i, 0, sourceIndex);
            if (this->remote_dir_model->isDir(currIndex)) {
                matches << this->remote_dir_model->filePath(currIndex);
            }
        }
        this->uiw->widget->onSetCompleteList(prefix, matches);
    }

}

void SFTPView::slot_dir_nav_go_home()
{
    q_debug()<<"";
    // check if current index is home index
    // if no, callepse all and expand to home
    // tell dir nav instance the home path

    QModelIndex proxyIndex = this->uiw->tableView->rootIndex();
    QModelIndex sourceIndex = this->m_tableProxyModel->mapToSource(proxyIndex);
    QModelIndex useIndex = sourceIndex;
    
    QString rootPath = this->remote_dir_model->filePath(sourceIndex);
    if (rootPath != this->user_home_path) {
        this->uiw->treeView->collapseAll();
        this->expand_to_home_directory(QModelIndex(), 1);
        this->uiw->tableView->setRootIndex(this->remote_dir_model->index(this->user_home_path));
    }
    this->uiw->widget->onSetHome(this->user_home_path);
}

void SFTPView::slot_dir_nav_prefix_changed(const QString &prefix)
{
    // q_debug()<<""<<prefix;
    QStringList matches;
    QModelIndex sourceIndex = this->remote_dir_model->index(prefix);
    QModelIndex currIndex;

    if (prefix == "") {
        sourceIndex = this->remote_dir_model->index(0, 0, QModelIndex());
    } else if (!sourceIndex.isValid()) {
        int pos = prefix.lastIndexOf('/');
        if (pos == -1) {
            pos = prefix.lastIndexOf('\\');
        }
        if (pos == -1) {
            // how deal this case
            Q_ASSERT(pos >= 0);
        }
        sourceIndex = this->remote_dir_model->index(prefix.left(prefix.length() - pos));
    }
    if (sourceIndex.isValid()) {
        if (this->remote_dir_model->canFetchMore(sourceIndex)) {
            while (this->remote_dir_model->canFetchMore(sourceIndex)) {
                this->is_dir_complete_request = true;
                this->dir_complete_request_prefix = prefix;
                this->remote_dir_model->fetchMore(sourceIndex);
            }
            // sience qt < 4.7 has no directoryLoaded signal, so we should execute now if > 0
            if (strcmp(qVersion(), "4.7.0") < 0) {
                // QTimer::singleShot(this, SLOT(
            }
        } else {
            int rc = this->remote_dir_model->rowCount(sourceIndex);
            q_debug()<<"lazy load dir rows:"<<rc;
            for (int i = rc - 1; i >= 0; --i) {
                currIndex = this->remote_dir_model->index(i, 0, sourceIndex);
                if (this->remote_dir_model->isDir(currIndex)) {
                    matches << this->remote_dir_model->filePath(currIndex);
                }
            }
            this->uiw->widget->onSetCompleteList(prefix, matches);
        }
    } else {
        // any operation for this case???
        q_debug()<<"any operation for this case???"<<prefix;
    }
}

void SFTPView::slot_dir_nav_input_comfirmed(const QString &prefix)
{
    q_debug()<<prefix;

    QModelIndex sourceIndex = this->remote_dir_model->index(prefix);
    QModelIndex proxyRootIndex = this->uiw->tableView->rootIndex(); 
    QModelIndex currIndex = this->m_tableProxyModel->mapToSource(proxyRootIndex);
    QString currPath, useIndex;

    if (!sourceIndex.isValid()) {
        q_debug()<<"directory not found!!!"<<prefix;
        // TODO, show status???
        this->status_bar->showMessage(tr("Directory not found: %1").arg(prefix));
        return;
    }

    // q_debug()<<currIndex.isValid();
    if (!currIndex.isValid()) {
        // some case will be !isValid(), eg. the tableView has been set to a invalid QModelIndex
        currPath = "";
    } else {
        currPath = this->remote_dir_model->filePath(currIndex);
    }
    QModelIndex proxyIndex = this->m_tableProxyModel->mapFromSource(sourceIndex);
    q_debug()<<currPath.length()<<currPath.isEmpty()<<currPath;
    if (currPath != prefix) {
        this->uiw->treeView->collapseAll();
        this->expand_to_directory(prefix, 1);
        this->uiw->tableView->setRootIndex(proxyIndex);
        this->uiw->listView_2->setRootIndex(proxyIndex);
    }
}

void SFTPView::encryption_focus_label_double_clicked()
{
    //qDebug()<<__FILE__<<":"<<__LINE__;
    EncryptionDetailDialog *enc_dlg = 0;
    char **server_info, **pptr, *p;
    int sftp_version;

	server_info = (char**)malloc(10 * sizeof(char*));
	for (int i = 0; i < 10; i++) {
		server_info[i] = (char*)malloc(512);
	}
    pptr = server_info = libssh2_session_get_remote_info(this->conn->sess, server_info);
    sftp_version = libssh2_sftp_get_version(this->ssh2_sftp); 

    enc_dlg = new EncryptionDetailDialog(server_info, this);
    enc_dlg->exec();

    //if(server_info != NULL) free(server_info);
    delete enc_dlg;

	for (int i = 0; i < 10; i++) {
		p = server_info[i];
		free(p);
	}
    free(server_info);
}

void SFTPView::host_info_focus_label_double_clicked()
{
    HostInfoDetailFocusLabel *hi_label = (HostInfoDetailFocusLabel*)sender();
    qDebug()<<"hehe"<<hi_label;

    LIBSSH2_CHANNEL *ssh2_channel = 0;
    int rv = -1;
    char buff[1024] ;
    QString evn_output;
    QString uname_output;
    const char *cmd = "uname -a";

    ssh2_channel = libssh2_channel_open_session(this->conn->sess);
    //libssh2_channel_set_blocking(ssh2_channel, 1);
    rv = libssh2_channel_exec(ssh2_channel, cmd);
    qDebug()<<"SSH2 exec: "<<rv;
  
    memset(buff, 0, sizeof(buff));
    while ((rv = libssh2_channel_read(ssh2_channel, buff, 1000)) > 0) {
        qDebug()<<"Channel read: "<<rv<<" -->"<<buff;
        uname_output += QString(buff);
        memset(buff, 0, sizeof(buff));
    }

    libssh2_channel_close(ssh2_channel);
    libssh2_channel_free(ssh2_channel);
    
    qDebug()<<"Host Info: "<<uname_output;
    hi_label->setToolTip(uname_output);

    SystemInfoDialog *sysInfoDlg = new SystemInfoDialog(this);    
    sysInfoDlg->setSystemInfo(uname_output);
    sysInfoDlg->exec();
    delete sysInfoDlg;
}

