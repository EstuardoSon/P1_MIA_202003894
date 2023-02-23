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
        this->reporteDisk();
    }
    else if(this->name == "sb"){
        this->reporteSb();
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

//Generar Reporte Disk
void Reporte::reporteDisk() {
    if(this->fichero_p != "" && this->archivo_p != "" && verificarRuta(this->fichero_p,this->archivo_p,"[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ]+")){
        NodoMount * nodo = this->listaMount->buscar(this->id);

        if (nodo != NULL) {
            FILE *archivoDisco = fopen((nodo->fichero + "/" + nodo->nombre_disco).c_str(), "rb");

            if (archivoDisco != NULL) {
                //Creacion de los ficheros y dando permisos
                string comando = "sudo -S mkdir -p \'" + this->fichero_p + "\'";
                system(comando.c_str());
                comando = "sudo -S chmod -R 777  \'" + this->fichero_p + "\'";
                system(comando.c_str());

                MBR mbr;
                fread(&mbr, sizeof(MBR), 1, archivoDisco);

                FILE *archivoReporte = fopen((this->fichero_p + "/reporteDisk.dot").c_str(), "w+");

                fprintf(archivoReporte, "digraph G {\n");
                fprintf(archivoReporte, "node[shape=none]\n");
                fprintf(archivoReporte, "start[label=<<table><tr>");
                fprintf(archivoReporte, "<td rowspan=\"2\">MBR</td>");

                int tamanioT = mbr.mbr_tamano;
                int inicio = sizeof(MBR);
                for (int i = 0; i < 4; i++) {
                    if (mbr.mbr_partition_[i].part_start != -1) {
                        if (mbr.mbr_partition_[i].part_type == 'P') {
                            float p1 = (mbr.mbr_partition_[i].part_s * 1.0) / tamanioT;
                            float porcentaje = p1 * 100.0;
                            string name1 = mbr.mbr_partition_[i].part_name;
                            fprintf(archivoReporte, "<td rowspan=\"2\">%s <br/>%.2f %</td>", name1.c_str(), porcentaje);
                            if (i != 3) {
                                int aux = i;
                                for (i = i + 1; i < 4; i++) {
                                    if (mbr.mbr_partition_[i].part_start != -1) {
                                        if ((mbr.mbr_partition_[aux].part_start + mbr.mbr_partition_[aux].part_s) <
                                            mbr.mbr_partition_[i].part_start) {
                                            porcentaje = ((mbr.mbr_partition_[i].part_start -
                                                           (mbr.mbr_partition_[aux].part_start +
                                                            mbr.mbr_partition_[aux].part_s) * 1.0) / tamanioT) * 100;
                                            fprintf(archivoReporte, "<td rowspan=\"2\">LIBRE <br/>%.2f %</td>",
                                                    porcentaje);
                                            i = i - 1;
                                            break;
                                        } else if (
                                                (mbr.mbr_partition_[aux].part_start + mbr.mbr_partition_[aux].part_s) ==
                                                mbr.mbr_partition_[i].part_start) {
                                            i = i - 1;
                                            break;
                                        }
                                    }
                                }
                                if (i == 4) {
                                    porcentaje =
                                            ((tamanioT -
                                              (mbr.mbr_partition_[aux].part_start + mbr.mbr_partition_[aux].part_s) *
                                              1.0) /
                                             tamanioT);
                                    porcentaje = porcentaje * 100.0;
                                    fprintf(archivoReporte, "<td rowspan=\"2\">LIBRE <br/>%.2f %</td>", porcentaje);
                                    goto t0;
                                }
                            } else if ((mbr.mbr_partition_[i].part_start + mbr.mbr_partition_[i].part_s) < tamanioT) {
                                porcentaje = ((tamanioT -
                                               (mbr.mbr_partition_[i].part_start + mbr.mbr_partition_[i].part_s) *
                                               1.0) / tamanioT) * 100;
                                fprintf(archivoReporte, "<td rowspan=\"2\">LIBRE <br/>%.2f %</td>", porcentaje);
                            }

                        } else if (mbr.mbr_partition_[i].part_type == 'E') {
                            float porcentaje = ((mbr.mbr_partition_[i].part_s) * 1.0 / tamanioT) * 100.0;
                            fprintf(archivoReporte, "<td rowspan=\"2\">EXTENDIDA</td>");
                            EBR ebr, ebrAux;
                            fseek(archivoDisco, mbr.mbr_partition_[i].part_start, SEEK_SET);
                            fread(&ebr, sizeof(EBR), 1, archivoDisco);
                            if (!(ebr.part_s == 0 && ebr.part_next == -1)) {
                                string name1 = ebr.part_name;
                                fprintf(archivoReporte, ("<td rowspan=\"2\">EBR <br/>" + name1 + "</td>").c_str());
                                porcentaje = ((ebr.part_s * 1.0) / tamanioT) * 100.0;
                                fprintf(archivoReporte, "<td rowspan=\"2\">Logica <br/>%.2f %</td>", porcentaje);

                                int finAnterior;
                                while (ebr.part_next != -1) {
                                    fseek(archivoDisco, ebr.part_next, SEEK_SET);
                                    fread(&ebrAux, sizeof(EBR), 1, archivoDisco);

                                    int finAnterior = ebr.part_s + ebr.part_start;
                                    if (ebrAux.part_start > finAnterior) {
                                        porcentaje = ((ebrAux.part_start * 1.0 - finAnterior) / tamanioT) * 100.0;
                                        fprintf(archivoReporte, "<td rowspan=\"2\">Libre <br/>%.2f %</td>", porcentaje);
                                    }

                                    name1 = ebrAux.part_name;
                                    fprintf(archivoReporte, ("<td rowspan=\"2\">EBR <br/>" + name1 + "</td>").c_str());
                                    porcentaje = ((ebrAux.part_s * 1.0) / tamanioT) * 100.0;
                                    fprintf(archivoReporte, "<td rowspan=\"2\">Logica <br/>%.2f %</td>", porcentaje);
                                    ebr = ebrAux;
                                }

                                finAnterior = ebr.part_s + ebr.part_start;
                                if (mbr.mbr_partition_[i].part_s > finAnterior) {
                                    porcentaje =
                                            ((mbr.mbr_partition_[i].part_s * 1.0 - finAnterior) / tamanioT) * 100.0;
                                    fprintf(archivoReporte, "<td rowspan=\"2\">Libre <br/>%.2f %</td>", porcentaje);
                                }
                            } else {
                                fprintf(archivoReporte, "<td rowspan=\"2\">Libre <br/>%.2f %</td>", porcentaje);
                            }
                            fprintf(archivoReporte, "<td rowspan=\"2\">FIN EXTENDIDA</td>");
                        }
                        inicio = mbr.mbr_partition_[i].part_start + mbr.mbr_partition_[i].part_s;
                    } else {
                        for (i = i + 1; i < 4; i++) {
                            if (mbr.mbr_partition_[i].part_start != -1) {
                                float porcentaje =
                                        ((mbr.mbr_partition_[i].part_start * 1.0 - inicio) / tamanioT) * 100.0;
                                fprintf(archivoReporte, "<td rowspan=\"2\">LIBRE <br/>%.2f %</td>", porcentaje);
                                i = i - 1;
                                break;
                            }
                        }
                        if (i == 4) {
                            float porcentaje = (((tamanioT - inicio) * 1.0) / tamanioT) * (100.0);
                            fprintf(archivoReporte, "<td rowspan=\"2\">LIBRE <br/>%.2f %</td>", porcentaje);
                            goto t0;
                        }
                    }
                }
                t0:
                fprintf(archivoReporte, "</tr></table>>];\n");
                fprintf(archivoReporte, "}");

                fclose(archivoDisco);
                fclose(archivoReporte);

                comando = "sudo -S dot -T" + this->obtenerExtension(this->archivo_p) + " " + this->fichero_p +
                          "/reporteDisk.dot -o \"" + this->fichero_p + "/" + this->archivo_p + "\"";
                system(comando.c_str());
                cout << "Reporte de DISK generado con Exito" << endl << endl;
            }   else{
                cout << "No fue posible encontrar el Disco... " << endl;
                cout << "Desmontando particion" << endl << endl;
                this->listaMount->eliminar(this->id);
            }
        }
    }
}

//Generar Reporte Sb
void Reporte::reporteSb() {
    if (this->fichero_p != "" && this->archivo_p != "" &&
        verificarRuta(this->fichero_p, this->archivo_p, "[a-zA-Z0-9_ñÑáéíóúÁÉÍÓÚ]+")) {
        NodoMount *nodo = this->listaMount->buscar(this->id);

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

                SuperBloque sb;
                bool verificar = false;
                for(int i = 0; i < 4; i++) {
                    if (strncmp(mbr.mbr_partition_[i].part_name, nodo->nombre_particion.c_str(), 16) == 0 &&
                        nodo->part_type == mbr.mbr_partition_[i].part_type) {
                        if (mbr.mbr_partition_[i].part_status == '2') {
                            fseek(archivoDisco, mbr.mbr_partition_[i].part_start, SEEK_SET);
                            fread(&sb, sizeof(SuperBloque), 1, archivoDisco);
                            verificar = true;
                            break;
                        } else if(mbr.mbr_partition_[i].part_status == '1') {
                            cout << "No se ha aplicado el comando MKFS a la Particion" << endl << endl;
                            goto t0;
                        }else if(mbr.mbr_partition_[i].part_status == '0') {
                            cout << "No se encontro la Particion en el Disco..." << endl;
                            cout << "Desmontando la Praticion" << endl << endl;
                            this->listaMount->eliminar(nodo->idCompleto);
                            goto t0;
                        }
                    } else if (mbr.mbr_partition_[i].part_type == 'E' &&
                               nodo->part_type == 'L') {
                        EBR ebr;
                        fseek(archivoDisco, nodo->part_start, SEEK_SET);
                        fread(&ebr, sizeof(EBR), 1, archivoDisco);

                        if (strncmp(ebr.part_name, nodo->nombre_particion.c_str(), 16) == 0) {
                            if (ebr.part_status == '2') {
                                fseek(archivoDisco, ebr.part_start + sizeof(EBR), SEEK_SET);
                                fread(&sb, sizeof(SuperBloque), 1, archivoDisco);
                                verificar = true;
                                break;
                            } else if (ebr.part_status == '1') {
                                cout << "No se ha aplicado el comando MKFS a la Particion" << endl << endl;
                                goto t0;
                            } else if (ebr.part_status == '0') {
                                cout << "No se encontro la Particion en el Disco..." << endl;
                                cout << "Desmontando la Praticion" << endl << endl;
                                this->listaMount->eliminar(nodo->idCompleto);
                                goto t0;
                            }
                        }
                    }
                }

                if(verificar){
                    FILE *archivoReporte = fopen((this->fichero_p + "/reporteSb.dot").c_str(), "w+");
                    fprintf(archivoReporte, "digraph G {\n");
                    fprintf(archivoReporte, "node[shape=none]\n");
                    fprintf(archivoReporte, "start[label=<<table>\n");
                    fprintf(archivoReporte,
                            "<tr><td colspan=\"2\" bgcolor=\"#145a32\"><font color=\"white\">REPORTE DE SUPERBLOQUE</font></td></tr>\n");

                    fprintf(archivoReporte,
                            ("<tr><td color=\"white\">sb_nombre_hd</td><td color=\"white\">" + nodo->nombre_disco +
                             "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td bgcolor=\"#27ae60\" color=\"white\">s_filesystem_type</td><td bgcolor=\"#27ae60\" color=\"white\">" +
                             to_string(sb.s_filesystem_type) + "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td color=\"white\">s_inodes_count</td><td color=\"white\">" + to_string(sb.s_inodes_count) +
                             "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td bgcolor=\"#27ae60\" color=\"white\">s_blocks_count</td><td bgcolor=\"#27ae60\" color=\"white\">" +
                             to_string(sb.s_blocks_count) + "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td color=\"white\">s_free_blocks_count</td><td color=\"white\">" + to_string(sb.s_free_blocks_count) +
                             "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td bgcolor=\"#27ae60\" color=\"white\">s_free_inodes_count</td><td bgcolor=\"#27ae60\" color=\"white\">" +
                             to_string(sb.s_free_inodes_count) + "</td></tr>\n").c_str());

                    tm *fecha = localtime(&sb.s_mtime);
                    char buffer[128];
                    strftime(buffer, sizeof(buffer), "%m-%d-%Y %X", fecha);
                    fprintf(archivoReporte,
                            ("<tr><td color=\"white\">s_mtime</td><td color=\"white\">" +
                             ((string) buffer) + "</td></tr>\n").c_str());

                    fecha = localtime(&sb.s_umtime);
                    char buffer2[128];
                    strftime(buffer2, sizeof(buffer2), "%m-%d-%Y %X", fecha);
                    fprintf(archivoReporte,
                            ("<tr><td bgcolor=\"#27ae60\" color=\"white\">s_umtime</td><td bgcolor=\"#27ae60\" color=\"white\">" +
                             ((string) buffer2) + "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td color=\"white\">s_mnt_count</td><td color=\"white\">" + to_string(sb.s_mnt_count) +
                             "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td bgcolor=\"#27ae60\" color=\"white\">s_magic</td><td bgcolor=\"#27ae60\" color=\"white\">" +
                             to_string(sb.s_magic) + "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td color=\"white\">s_inode_size</td><td color=\"white\">" + to_string(sb.s_inode_size) +
                             "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td bgcolor=\"#27ae60\" color=\"white\">s_block_size</td><td bgcolor=\"#27ae60\" color=\"white\">" +
                             to_string(sb.s_block_size) + "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td color=\"white\">s_first_ino</td><td color=\"white\">" + to_string(sb.s_first_ino) +
                             "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td bgcolor=\"#27ae60\" color=\"white\">s_first_blo</td><td bgcolor=\"#27ae60\" color=\"white\">" +
                             to_string(sb.s_first_blo) + "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td color=\"white\">s_bm_inode_start</td><td color=\"white\">" + to_string(sb.s_bm_inode_start) +
                             "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td bgcolor=\"#27ae60\" color=\"white\">s_bm_block_start</td><td bgcolor=\"#27ae60\" color=\"white\">" +
                             to_string(sb.s_bm_block_start) + "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td color=\"white\">s_inode_start</td><td color=\"white\">" + to_string(sb.s_inode_start) +
                             "</td></tr>\n").c_str());

                    fprintf(archivoReporte,
                            ("<tr><td bgcolor=\"#27ae60\" color=\"white\">s_block_start</td><td bgcolor=\"#27ae60\" color=\"white\">" +
                             to_string(sb.s_block_start) + "</td></tr>\n").c_str());

                    fprintf(archivoReporte, "</table>>];\n");
                    fprintf(archivoReporte, "}");

                    fclose(archivoReporte);
                    comando = "sudo -S dot -T" + this->obtenerExtension(this->archivo_p) + " " + this->fichero_p +
                              "/reporteSb.dot -o \"" + this->fichero_p + "/" + this->archivo_p + "\"";
                    system(comando.c_str());
                    cout << "Reporte de SB generado con Exito" << endl << endl;
                } else{
                    cout << "La Particion no exite dentro del disco..." << endl << endl;
                    cout << "Desmontando la particion" << endl << endl;
                    this->listaMount->eliminar(nodo->id);
                }
                t0:
                fclose(archivoDisco);
            } else{
                cout << "No fue posible encontrar el Disco... " << endl;
                cout << "Desmontando particion" << endl << endl;
                this->listaMount->eliminar(this->id);
            }
        }
    }
}