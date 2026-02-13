#include "SitrepTypeSupportImpl.h"
#include "DataReaderListenerImpl.h"
#include "DatabaseManager.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    try {
        // Inicializar DDS
        DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
        
        DDS::DomainParticipant_var participant = dpf->create_participant(42,
            PARTICIPANT_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        if (!participant) {
            std::cerr << "Error creando participant" << std::endl;
            return 1;
        }

        // Registrar Tipo
        DefenseData::SitrepMessageTypeSupport_var ts = new DefenseData::SitrepMessageTypeSupportImpl;
        if (ts->register_type(participant, "") != DDS::RETCODE_OK) {
            std::cerr << "Error registrando tipo" << std::endl;
            return 1;
        }

        // Crear Topic
        CORBA::String_var type_name = ts->get_type_name();
        DDS::Topic_var topic = participant->create_topic("Sitreps",
            type_name, TOPIC_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        if (!topic) {
            std::cerr << "Error creando topic" << std::endl;
            return 1;
        }

        std::string myName;
        std::cout << "Ingrese nombre de esta terminal: ";
        std::cin >> myName;

        DatabaseManager db;
        
        // --- MODIFICADO: Volvemos a usar un solo archivo compartido ---
        std::string dbName = "tactical_data.db";
        
        if (!db.init(dbName)) {
            std::cerr << "Error iniciando base de datos" << std::endl;
            return 1;
        }

        // Crear Suscriptor y Listener
        DDS::Subscriber_var sub = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
        DDS::DataReaderListener_var listener = new DataReaderListenerImpl(db);
        DDS::DataReader_var reader = sub->create_datareader(topic,
            DATAREADER_QOS_DEFAULT, listener, OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        // Crear Publicador
        DDS::Publisher_var pub = participant->create_publisher(PUBLISHER_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
        
        // --- NUEVO: Configurar QoS para NO borrar automáticamente al salir ---
        DDS::DataWriterQos writer_qos;
        pub->get_default_datawriter_qos(writer_qos);
        writer_qos.writer_data_lifecycle.autodispose_unregistered_instances = false;

        // Crear DataWriter usando nuestra configuración writer_qos (en vez de DATAWRITER_QOS_DEFAULT)
        DDS::DataWriter_var writer = pub->create_datawriter(topic,
            writer_qos, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        DefenseData::SitrepMessageDataWriter_var sitrepWriter =
            DefenseData::SitrepMessageDataWriter::_narrow(writer);

// --- BUCLE PRINCIPAL PROTEGIDO ---
        char input = 0;
        
        while (input != 'q') {
            std::cout << "\nComando (p: publicar, b: borrar, q: salir): ";
            
            if (!(std::cin >> input)) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                continue;
            }

            if (input == 'p') {
                DefenseData::SitrepMessage msg;
                msg.sourceId = myName.c_str(); 

                std::cout << "Ingrese Track ID (numero): "; 
                while (!(std::cin >> msg.trackId)) {
                    std::cin.clear();             
                    std::cin.ignore(10000, '\n'); 
                    std::cout << "Error: Ingresa un numero valido.\nTrack ID: ";
                }
                
                std::string tempIdentidad;
                std::cout << "Identidad (HOSTIL/AMIGO): "; 
                std::cin >> tempIdentidad;
                msg.identidad = tempIdentidad.c_str();
                
                std::cout << "Latitud (numero): "; 
                while (!(std::cin >> msg.latitud)) {
                    std::cin.clear();
                    std::cin.ignore(10000, '\n');
                    std::cout << "Error: Ingresa un numero valido.\nLatitud: ";
                }
                
                std::cout << "Longitud (numero): "; 
                while (!(std::cin >> msg.longitud)) {
                    std::cin.clear();
                    std::cin.ignore(10000, '\n');
                    std::cout << "Error: Ingresa un numero valido.\nLongitud: ";
                }
                
                std::cin.ignore(10000, '\n'); 
                std::cout << "Info Extra: "; 
                std::string tempInfo;
                std::getline(std::cin, tempInfo);
                msg.infoAmpliatoria = tempInfo.c_str();

                DDS::ReturnCode_t ret = sitrepWriter->write(msg, DDS::HANDLE_NIL);
                if (ret == DDS::RETCODE_OK) {
                    std::cout << ">>> SITREP ENVIADO <<<" << std::endl;
                } else {
                    std::cerr << "Error enviando sitrep: " << ret << std::endl;
                }
} else if (input == 'b') {
                DefenseData::SitrepMessage msg;
                msg.sourceId = myName.c_str();
                msg.identidad = "";
                msg.infoAmpliatoria = "";
                msg.latitud = 0.0;
                msg.longitud = 0.0;
                
                std::cout << "Ingrese Track ID a dar de baja: "; 
                
                while (!(std::cin >> msg.trackId)) {
                    std::cin.clear();
                    std::cin.ignore(10000, '\n');
                    std::cout << "Error: Ingresa un numero valido.\nTrack ID: ";
                }

                // --- EL SECRETO ESTÁ ACÁ ---
                // 1. Registramos la instancia para que el DataWriter sepa que existe
                DDS::InstanceHandle_t handle = sitrepWriter->register_instance(msg);

                // 2. Ahora sí, le mandamos la señal de dispose usando ese "handle" (identificador)
                DDS::ReturnCode_t ret = sitrepWriter->dispose(msg, handle);
                
                if (ret == DDS::RETCODE_OK) {
                    std::cout << ">>> ORDEN DE BAJA ENVIADA A LA RED <<<" << std::endl;
                    
                    // Lo borramos de nuestra base de datos local
                    db.deleteSitrep(msg.trackId);
                } else {
                    std::cerr << "Error enviando baja: " << ret << std::endl;
                }
            }                
        }
        // Limpieza
        participant->delete_contained_entities();
        dpf->delete_participant(participant);
        TheServiceParticipant->shutdown();

    } catch (const CORBA::Exception& e) {
        e._tao_print_exception("Excepcion CORBA en main: ");
        return 1;
    }
    return 0;
}
