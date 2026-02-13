#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <sqlite3.h>
#include <string>
#include <iostream>
#include "SitrepC.h" 

class DatabaseManager {
    sqlite3* db;

public:
    DatabaseManager() : db(nullptr) {}

    bool init(const std::string& dbName) {
        if (sqlite3_open(dbName.c_str(), &db) != SQLITE_OK) {
            std::cerr << "[DB Error] No se pudo abrir la DB: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }

        // Configuración para evitar bloqueos
        sqlite3_busy_timeout(db, 5000); 
        char* errMsg = 0;
        sqlite3_exec(db, "PRAGMA journal_mode=WAL;", 0, 0, &errMsg);

        const char* sql = "CREATE TABLE IF NOT EXISTS sitreps ("
                          "trackId INTEGER PRIMARY KEY, "
                          "sourceId TEXT, "
                          "identidad TEXT, "
                          "latitud REAL, "
                          "longitud REAL, "
                          "infoAmpliatoria TEXT);";

        if (sqlite3_exec(db, sql, 0, 0, &errMsg) != SQLITE_OK) {
            std::cerr << "[DB Error] Init SQL: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            return false;
        }
        return true;
    }

    void upsertSitrep(const DefenseData::SitrepMessage& msg) {
        std::string sql = "INSERT OR REPLACE INTO sitreps (trackId, sourceId, identidad, latitud, longitud, infoAmpliatoria) "
                          "VALUES (?, ?, ?, ?, ?, ?);";

        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) {
            std::cerr << "[DB Error] Prepare: " << sqlite3_errmsg(db) << std::endl;
            return;
        }

        sqlite3_bind_int(stmt, 1, msg.trackId);
        sqlite3_bind_text(stmt, 2, msg.sourceId.in(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, msg.identidad.in(), -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 4, msg.latitud);
        sqlite3_bind_double(stmt, 5, msg.longitud);
        sqlite3_bind_text(stmt, 6, msg.infoAmpliatoria.in(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "[DB Error] Step: " << sqlite3_errmsg(db) << std::endl;
        }
        
        sqlite3_finalize(stmt);
    }

    // --- NUEVA FUNCIÓN PARA BORRAR ---
    void deleteSitrep(long trackId) {
        std::string sql = "DELETE FROM sitreps WHERE trackId = ?;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) {
            std::cerr << "[DB Error] Prepare delete: " << sqlite3_errmsg(db) << std::endl;
            return;
        }

        sqlite3_bind_int(stmt, 1, trackId);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "[DB Error] Step delete: " << sqlite3_errmsg(db) << std::endl;
        }
        
        sqlite3_finalize(stmt);
    }

    ~DatabaseManager() {
        if (db) sqlite3_close(db);
    }
};

#endif
