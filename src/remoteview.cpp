/***************************************************************************
 *   Copyright (C) 2007 by liuguangzhao   *
 *   gzl@localhost   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QtCore>


#include "remoteview.h"


#include "sftp-operation.h"


RemoteView::RemoteView(QWidget *parent)
 : QWidget(parent)
{
    this->remoteview.setupUi(this);
    
    memset(this->m_curr_path,0,sizeof(this->m_curr_path));
    memset(this->m_next_path,0,sizeof(this->m_next_path) );
    this->m_curr_path[0] = '/';
    this->m_next_path[0] = '/';
    
    this->remoteview.treeView->setAcceptDrops(true);
    this->remoteview.treeView->setDragEnabled(true);
    this->remoteview.treeView->setDropIndicatorShown(true);
    this->remoteview.treeView->setDragDropMode(QAbstractItemView::DragDrop);
    //this->remoteview.treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    
    QObject::connect(this->remoteview.treeView,SIGNAL(customContextMenuRequested(const QPoint &)),
                     this,SLOT(slot_dir_tree_customContextMenuRequested (const QPoint & )) );
    this->init_popup_context_menu();
}

void RemoteView::init_popup_context_menu()
{

    this->dir_tree_context_menu = new QMenu();
    QAction *action ;
    action  = new QAction("transfer",0);
    this->dir_tree_context_menu->addAction(action);
    QObject::connect(action,SIGNAL(triggered()),this,SLOT(slot_new_transfer()));
    
}

RemoteView::~RemoteView()
{
}
void RemoteView::slot_show_fxp_command_log(bool show)
{
    this->remoteview.listView->setVisible(show);    
}

void RemoteView::i_init_dir_view(struct sftp_conn * conn)
{
    qDebug()<<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;

    this->remote_dir_model = new RemoteDirModel(conn);
    
    this->remoteview.treeView->setModel(this->remote_dir_model);
    this->remoteview.treeView->setAcceptDrops(true);
    this->remoteview.treeView->setDragEnabled(true);
    this->remoteview.treeView->setDropIndicatorShown(true);
    this->remoteview.treeView->setDragDropMode(QAbstractItemView::DragDrop);            
    //do_globbed_ls( conn , this->m_next_path , this->m_curr_path, 0 );
    QObject::connect(this->remote_dir_model,SIGNAL(new_transfer_requested(QString,QString,QString,QString)),
                     this,SIGNAL(new_transfer_requested(QString,QString,QString,QString)) ) ;
}

void RemoteView::slot_disconnect_from_remote_host()
{
    this->remoteview.treeView->setModel(0);
    delete this->remote_dir_model ;
    this->remote_dir_model = 0 ;
}

void RemoteView::slot_dir_tree_customContextMenuRequested ( const QPoint & pos )
{
    
    this->dir_tree_context_menu->popup(this->mapToGlobal(pos));
}

void RemoteView::slot_new_transfer()
{
    qDebug()<<__FUNCTION__<<": "<<__LINE__<<":"<< __FILE__;
     
    QString file_path , file_type ;
    
    
    QItemSelectionModel *ism = this->remoteview.treeView->selectionModel();
    
    QModelIndexList mil = ism->selectedIndexes()   ;
    
    for(int i = 0 ; i < mil.size(); i +=4 )
    {
        QModelIndex midx = mil.at(i);
        qDebug()<<midx ;
        directory_tree_item * dti = (directory_tree_item*) midx.internalPointer();
        qDebug()<<dti->file_name.c_str()<<" "<<dti->file_type.c_str()
                <<" "<< dti->strip_path.c_str() ;
        file_path = dti->strip_path.c_str();
        file_type = dti->file_type.c_str() ;
    }
    
    //emit new_transfer_requested("/vmlinuz-2.6.18.2-34-xen");
    emit new_transfer_requested(file_path,file_type );
    
}

QString RemoteView::get_selected_directory()
{
    QString file_path ;
    
    QItemSelectionModel *ism = this->remoteview.treeView->selectionModel();
    
    if(ism == 0)
    {
        return file_path ;
    }
    
    QModelIndexList mil = ism->selectedIndexes()   ;
    
    for(int i = 0 ; i < mil.size(); i +=4 )
    {
        QModelIndex midx = mil.at(i);
        qDebug()<<midx ;
        directory_tree_item * dti = (directory_tree_item*) midx.internalPointer();
        qDebug()<<dti->file_name.c_str()<<" "<<dti->file_type.c_str()
                <<" "<< dti->strip_path.c_str() ;
        file_path = dti->strip_path.c_str();
    }
    
    return file_path ;

}







