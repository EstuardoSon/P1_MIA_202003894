//
// Created by estuardo on 13/02/23.
//

#include "Reporte.h"
#include <iostream>
#include <regex>
#include <cmath>

Reporte::Reporte(ListaMount * listaMount) {
    this->listaMount = listaMount;
    this->name = "";
    this->fichero_p = "";
    this->archivo_p = "";
    this->fichero_r = "";
    this->archivo_r = "";
    this->id = "";
    this->ruta = "";
}

bool Reporte::verificarRuta(std::string fichero, std::string archivo, std::string extension) {
    regex expresion ("\\/(([a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\/))*[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ ]+(\\."+extension+")?)?");
    if (!regex_match(fichero+"/"+archivo,expresion)) {
        cout << "La direccion ingresada no es valida o la extension del archivo no es la adecuada" << endl << endl;
        return false;
    }
    return true;
}

//Obtener Extension Archivo
string Reporte::obtenerExtension(std::string nombreArchivo) {
    string delimitador = ".";
    size_t pos = 0;
    string extension;
    while (pos = nombreArchivo.find(delimitador) != string::npos) {
        extension = nombreArchivo.erase(0, pos);
    }
    return extension;
}

//Generacion de Reportes
void Reporte::generarReporte() {
    if (this->name == "mbr"){
        this->reporteMBR();
    }
    else if (this->name == "disk"){
    }
    else if(this->name == "sb"){
    }
    else if(this->name == "bm_inode"){
    }
    else if(this->name == "bm_bloc"){
    }
    else if(this->name == "inode"){
    }
    else if(this->name == "block"){
    }
    else if(this->name == "tree"){
    }
    else if(this->name == "ls"){
    }
    else if(this->name == "file"){
    }
}

//Generar Celdas de Particiones Primarias y Extendidas para Reporte MBR
void Reporte::crearParticionRMBR(FILE *archivoReporte, FILE *archivoDisco, Partition &partition) {
    fprintf(archivoReporte,
            "<tr><td colspan=\"2\" bgcolor=\"#4B0082\"><font color=\"white\">Particion</font></td></tr>\n");
    string cadena = "";
    cadena += partition.part_status;
    fprintf(archivoReporte,
            ("<tr><td color=\"white\">part_status</td><td color=\"white\">" + cadena + "</td></tr>\n").c_str());
    cadena = "";
    cadena += partition.part_type;
    fprintf(archivoReporte,
            (R"(  <tr><td bgcolor="#6A5ACD" color="white">part_type</td><td bgcolor="#6A5ACD" color="white">)" +
             cadena + "</td></tr>\n").c_str());
    cadena = "";
    cadena += partition.part_fit;
    fprintf(archivoReporte, "%s", (R"(<tr><td color="white">part_fit</td><td color="white">)" +
                                   cadena + "</td></tr>\n").c_str());
    fprintf(archivoReporte,
            (R"(  <tr><td bgcolor="#6A5ACD" color="white">part_start</td><td bgcolor="#6A5ACD" color="white">)" +
             to_string(partition.part_start) + "</td></tr>\n").c_str());

    fprintf(archivoReporte, "%s", (R"(<tr><td color="white">part_s</td><td color="white">)" +
                                   to_string(partition.part_s) + "</td></tr>\n").c_str());

    string nombre = "";
    nombre += partition.part_name;
    fprintf(archivoReporte,
            (R"(  <tr><td bgcolor="#6A5ACD" color="white">part_name</td><td bgcolor="#6A5ACD" color="white">)" +
             nombre + "</td></tr>\n").c_str());

    if (partition.part_type == 'E') {
        this->crearParticionLRMBR(archivoReporte, archivoDisco, partition.part_start);
    }
}

//Generar Celdas de Particiones Logicas para Reporte MBR
void Reporte::crearParticionLRMBR(FILE *archivoReporte, FILE *archivoDisco, int part_start) {
    EBR ebr;
    fseek(archivoDisco, part_start, SEEK_SET);
    fread(&ebr, sizeof(EBR), 1, archivoDisco);

    fprintf(archivoReporte,
            "<tr><td colspan=\"2\" bgcolor=\"#D87093\"><font color=\"white\">Particion Logica</font></td></tr>\n");

    string cadena = "";
    cadena += ebr.part_status;
    fprintf(archivoReporte,
            ("<tr><td color=\"white\">part_status</td><td color=\"white\">" + cadena + "</td></tr>\n").c_str());
    fprintf(archivoReporte,
            (R"(  <tr><td bgcolor="#FFC0CB" color="white">part_next</td><td bgcolor="#FFC0CB" color="white">)" +
             to_string(ebr.part_next) + "</td></tr>\n").c_str());
    cadena = "";
    cadena += ebr.part_fit;
    fprintf(archivoReporte, "%s", (R"(<tr><td color="white">part_fit</td><td color="white">)" +
                                   cadena + "</td></tr>\n").c_str());
    fprintf(archivoReporte,
            (R"(  <tr><td bgcolor="#FFC0CB" color="white">part_start</td><td bgcolor="#FFC0CB" color="white">)" +
             to_string(ebr.part_start) + "</td></tr>\n").c_str());

    fprintf(archivoReporte, "%s", (R"(<tr><td color="white">part_s</td><td color="white">)" +
                                   to_string(ebr.part_s) + "</td></tr>\n").c_str());

    string nombre = "";
    nombre += ebr.part_name;
    fprintf(archivoReporte,
            (R"(  <tr><td bgcolor="#FFC0CB" color="white">part_name</td><td bgcolor="#FFC0CB" color="white">)" +
             nombre + "</td></tr>\n").c_str());

    if (ebr.part_next != -1) {
        this->crearParticionLRMBR(archivoReporte, archivoDisco, ebr.part_next);
    }
}

//Generar Reporte MBR
void Reporte::reporteMBR() {
    if(this->fichero_p != "" && this->archivo_p != "" && verificarRuta(this->fichero_p,this->archivo_p,"[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ]+")){
        NodoMount * nodo = this->listaMount->buscar(this->id);

        if (nodo != NULL) {
            FILE *archivoDisco = fopen((nodo->fichero + "/" + nodo->nombre_disco).c_str(), "rb");

            if(archivoDisco != NULL) {
                //Creacion de los ficheros y dando permisos
                string comando = "sudo -S mkdir -p \'" + this->fichero_p + "\'";
                system(comando.c_str());
                comando = "sudo -S chmod -R 777  \'" + this->fichero_p + "\'";
                system(comando.c_str());

                MBR mbr;
                fread(&mbr, sizeof(MBR), 1, archivoDisco);

                FILE *archivoReporte = fopen((this->fichero_p + "/reporteMBR.dot").c_str(), "w+");
                fprintf(archivoReporte, "digraph G {\n");
                fprintf(archivoReporte, "node[shape=none]\n");
                fprintf(archivoReporte, "start[label=<<table>\n");
                fprintf(archivoReporte,
                        "<tr><td colspan=\"2\" bgcolor=\"#4B0082\"><font color=\"white\">REPORTE DE MBR</font></td></tr>\n");

                fprintf(archivoReporte,
                        (R"(<tr><td color="white">mbr_tamano</td><td color="white">)" + to_string(mbr.mbr_tamano) +
                         "</td></tr>\n").c_str());

                tm *fechaCreacion = localtime(&mbr.mbr_fecha_creacion);
                char buffer[128];
                strftime(buffer, sizeof(buffer), "%m-%d-%Y %X", fechaCreacion);
                fprintf(archivoReporte,
                        (R"(  <tr><td bgcolor="#6A5ACD" color="white">mbr_fecha_creacion</td><td bgcolor="#6A5ACD" color="white">)" +
                         ((string) buffer) + "</td></tr>\n").c_str());

                fprintf(archivoReporte, "%s", (R"(<tr><td color="white">mbr_disk_signature</td><td color="white">)" +
                                               to_string(mbr.mbr_dsk_signature) + "</td></tr>\n").c_str());

                for (int i = 0; i < 4; i++) {
                    if (mbr.mbr_partition_[i].part_start != -1) {
                        this->crearParticionRMBR(archivoReporte, archivoDisco, mbr.mbr_partition_[i]);
                    }
                }

                fprintf(archivoReporte, "</table>>];\n");
                fprintf(archivoReporte, "}");

                fclose(archivoDisco);
                fclose(archivoReporte);

                comando = "sudo -S dot -T" + this->obtenerExtension(this->archivo_p) + " " + this->fichero_p +
                          "/reporteMBR.dot -o \"" + this->fichero_p + "/" + this->archivo_p + "\"";
                system(comando.c_str());
                cout << "Reporte de MBR generado con Exito" << endl << endl;
            } else{
                cout << "No fue posible encontrar el Disco... " << endl;
                cout << "Desmontando particion" << endl << endl;
                this->listaMount->eliminar(this->id);
            }
        }
    }
}
