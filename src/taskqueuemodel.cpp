// taskqueuemodel.cpp --- 
// 
// Author: liuguangzhao
// Copyright (C) 2007-2010 liuguangzhao@users.sf.net
// URL: 
// Created: 2010-03-25 10:35:29 +0800
// Version: $Id$
// 

#include "taskqueuemodel.h"


TaskQueueModel *TaskQueueModel::inst = 
    new TaskQueueModel(0, QSqlDatabase::addDatabase("QSQLITE", "idtq"));

TaskQueueModel::TaskQueueModel(QObject *parent, QSqlDatabase db)
    : QSqlTableModel(parent, db), inited(false)
{
    QSqlDatabase taskQueueDb = QSqlDatabase::database("idtq", false);
    taskQueueDb.setDatabaseName("tobememory");
    // taskQueueDb.setDatabaseName(":memory:");
    if (! taskQueueDb.open()) {
        qDebug()<<"open sqlite error";
    } else {
        qDebug()<<"open sqlite okkkkkkkkkkkkk";
    }

}

TaskQueueModel::~TaskQueueModel()
{
    this->finalize();
}

TaskQueueModel *TaskQueueModel::instance()
{
    TaskQueueModel *q = TaskQueueModel::inst;
    if (!q->inited) {
        q->init();
        q->inited = true;
    }
    return q;
}

void TaskQueueModel::init()
{
    int rv = sqlite3_open_v2("tobememory", &this->pDB, 
                             SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_SHAREDCACHE,
                             NULL);
    if (rv != SQLITE_OK) {
        qDebug()<<"Sqlite error:"<<rv<<":"<<sqlite3_errmsg(this->pDB);
        sqlite3_close(this->pDB);
        this->pDB = NULL;
    }
    Q_ASSERT(rv == SQLITE_OK);

    // 创建表，
    char * createTable = 
        " CREATE TABLE IF NOT EXISTS task_queue ("
        "  id INTEGER PRIMARY KEY,"
        "  file_name VARCHAR(128),"
        "  dest_path VARCHAR(256),"
        "  file_size UNSIGNED BIG INT,"
        "  got_size  UNSIGNED BIG INT,"
        "  got_percent INT2,"
        "  eclapsed_time VARCHAR(10),"
        "  left_time VARCHAR(10),"
        "  speed VARCHAR(10),"
        "  status VARCHAR(10),"
        "  start_time VARCHAR(16),"
        "  finish_time VARCHAR(16)"
        "); DELETE FROM task_queue WHERE 1;";

    rv = sqlite3_exec(this->pDB, createTable, NULL, NULL, NULL);
    if (rv != SQLITE_OK) {
        qDebug()<<"Sqlite error:"<<rv<<":"<<sqlite3_errmsg(this->pDB);
    }
    Q_ASSERT(rv == SQLITE_OK);
}

void TaskQueueModel::finalize()
{
    int rv = sqlite3_close(this->pDB);
    if (rv != SQLITE_OK) {
        qDebug()<<"Sqlite error:"<<rv<<":"<<sqlite3_errmsg(this->pDB);
    }
    Q_ASSERT(rv == SQLITE_OK);
}

void TaskQueueModel::slot_set_transfer_percent(int percent, int total_transfered, int transfer_delta)
{
    
}
void TaskQueueModel::slot_transfer_thread_finished()
{
}
void TaskQueueModel::slot_new_file_transfer_started(QString new_file_name)
{
    // char *preSql = "INSERT INTO task_queue (id, file_name, dest_path, start_time) VALUES (NULL, '%s', '%s', '%s');";
    // sqlite3_stmt *pStmt = NULL;
    // char sqlBuff[1024] = {0};

    // sprintf(sqlBuff, preSql, QFileInfo(new_file_name).baseName().toAscii().data(),
    //         new_file_name.toAscii().data(),
    //         QDateTime::currentDateTime().toString().toAscii().data());

    // qDebug()<<"SQL:"<<sqlBuff;
    // int rv = sqlite3_exec(this->pDB, sqlBuff, 0, 0, 0);
    // if (rv != SQLITE_OK) {
    //     qDebug()<<"Sqlite error:"<<rv<<":"<<sqlite3_errmsg(this->pDB);
    // }
    // Q_ASSERT(rv == SQLITE_OK);
    // qDebug()<<" rowid: "<<sqlite3_last_insert_rowid(this->pDB);
    // emit this->insertRow(sqlite3_last_insert_rowid(this->pDB)-1);

    this->insertRows(0, 1);
    this->setData(this->index(0, 1), QFileInfo(new_file_name).baseName());
    this->setData(this->index(0, 2), new_file_name);
    this->setData(this->index(0, 10), QDateTime::currentDateTime().toString());
    this->submit();

    // int rv = sqlite3_exec(this->pDB, "BEGIN TRANSACTION;", 0, 0, 0);
    // rv = sqlite3_prepare_v2(this->pDB, preSql, -1, &pStmt, NULL);
    // if (rv != SQLITE_OK) {
    //     qDebug()<<"Sqlite error:"<<rv<<":"<<sqlite3_errmsg(this->pDB);
    // }
    // Q_ASSERT(rv == SQLITE_OK);
    
    // rv = sqlite3_bind_text(pStmt, 1, "abcd", //QFileInfo(new_file_name).baseName().toAscii().data(),
    //                        4, // QFileInfo(new_file_name).baseName().length(),
    //                        NULL);
    // if (rv != SQLITE_OK) {
    //     qDebug()<<"Sqlite error:"<<rv<<":"<<sqlite3_errmsg(this->pDB);
    // }
    // Q_ASSERT(rv == SQLITE_OK);

    // rv = sqlite3_bind_text(pStmt, 2, "abcde", // new_file_name.toAscii().data(),
    //                        5,// new_file_name.length(), 
    //                        NULL);
    // if (rv != SQLITE_OK) {
    //     qDebug()<<"Sqlite error:"<<rv<<":"<<sqlite3_errmsg(this->pDB);
    // }
    // Q_ASSERT(rv == SQLITE_OK);

    // rv = sqlite3_bind_text(pStmt, 3, "else", //QDateTime::currentDateTime().toString().toAscii().data(),
    //                        4, // QDateTime::currentDateTime().toString().length(), 
    //                        NULL);
    // if (rv != SQLITE_OK) {
    //     qDebug()<<"Sqlite error:"<<rv<<":"<<sqlite3_errmsg(this->pDB);
    // }
    // Q_ASSERT(rv == SQLITE_OK);

    // rv = sqlite3_step(pStmt);
    // if (rv != SQLITE_OK) {
    //     qDebug()<<"Sqlite error:"<<rv<<":"<<sqlite3_errmsg(this->pDB);
    // }
    // Q_ASSERT(rv == SQLITE_OK);

    // preSql = (char*)sqlite3_sql(pStmt);
    // qDebug()<<"Stmt SQL: "<<preSql<<" rowid: "<<sqlite3_last_insert_rowid(this->pDB);

    // {
    //     rv = sqlite3_reset(pStmt);
    //     if (rv != SQLITE_OK) {
    //         qDebug()<<"Sqlite error:"<<rv<<":"<<sqlite3_errmsg(this->pDB);
    //     }
    //     Q_ASSERT(rv == SQLITE_OK);
    // }

    // rv = sqlite3_finalize(pStmt);
    // if (rv != SQLITE_OK) {
    //     qDebug()<<"Sqlite error:"<<rv<<":"<<sqlite3_errmsg(this->pDB);
    // }
    // Q_ASSERT(rv == SQLITE_OK);

    // rv = sqlite3_exec(this->pDB, "COMMIT TRANSACTION;", 0, 0, 0);
}

void TaskQueueModel::slot_transfer_got_file_size(int size)
{
}
