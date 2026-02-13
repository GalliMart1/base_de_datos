#include "DataReaderListenerImpl.h"
#include "SitrepTypeSupportC.h"
#include "SitrepC.h"
#include <iostream>

DataReaderListenerImpl::DataReaderListenerImpl(DatabaseManager& db)
  : dbMgr(db)
{
}

DataReaderListenerImpl::~DataReaderListenerImpl()
{
}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  DefenseData::SitrepMessageDataReader_var reader_i =
    DefenseData::SitrepMessageDataReader::_narrow(reader);

  if (!reader_i) {
    std::cerr << "Error: _narrow failed en on_data_available" << std::endl;
    return;
  }

  DefenseData::SitrepMessage msg;
  DDS::SampleInfo info;

  while (true) {
    DDS::ReturnCode_t error = reader_i->take_next_sample(msg, info);
    
    if (error == DDS::RETCODE_NO_DATA) {
        break;
    } 
    else if (error != DDS::RETCODE_OK) {
        std::cerr << "Error al leer sample: " << error << std::endl;
        break;
    }

    if (info.valid_data) {
        // --- 1. DATOS NUEVOS O ACTUALIZADOS ---
        std::cout << "\n>>> [SITREP RECIBIDO DE " << msg.sourceId.in() << "] <<<" << std::endl;
        std::cout << "    Track ID: " << msg.trackId << " | Identidad: " << msg.identidad.in() << std::endl;
        std::cout << "    Pos: " << msg.latitud << ", " << msg.longitud << std::endl;
        std::cout << "    Info: " << msg.infoAmpliatoria.in() << std::endl;
        std::cout << "-----------------------------------" << std::endl;
        std::cout << "Comando (p: publicar, b: borrar, q: salir): ";
        std::cout.flush();

        dbMgr.upsertSitrep(msg);
        
    } else if (info.instance_state & DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
        // --- 2. SEÑAL DE BORRADO RECIBIDA ---
        // Aunque valid_data es false, OpenDDS guarda la Key (trackId) en 'msg'
        std::cout << "\n>>> [ALERTA: SITREP ELIMINADO EN LA RED] <<<" << std::endl;
        std::cout << "    El Track ID: " << msg.trackId << " ha sido dado de baja." << std::endl;
        std::cout << "-----------------------------------" << std::endl;
        std::cout << "Comando (p: publicar, b: borrar, q: salir): ";
        std::cout.flush();

        // Eliminar de la base de datos de SQLite
        dbMgr.deleteSitrep(msg.trackId);
    }
  }
}

// ... (Las demás funciones on_requested_deadline_missed, etc., quedan igual de vacías) ...
void DataReaderListenerImpl::on_requested_deadline_missed(DDS::DataReader_ptr, const DDS::RequestedDeadlineMissedStatus&) {}
void DataReaderListenerImpl::on_requested_incompatible_qos(DDS::DataReader_ptr, const DDS::RequestedIncompatibleQosStatus&) {}
void DataReaderListenerImpl::on_sample_rejected(DDS::DataReader_ptr, const DDS::SampleRejectedStatus&) {}
void DataReaderListenerImpl::on_liveliness_changed(DDS::DataReader_ptr, const DDS::LivelinessChangedStatus&) {}
void DataReaderListenerImpl::on_subscription_matched(DDS::DataReader_ptr, const DDS::SubscriptionMatchedStatus&) {}
void DataReaderListenerImpl::on_sample_lost(DDS::DataReader_ptr, const DDS::SampleLostStatus&) {}
