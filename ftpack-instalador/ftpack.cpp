#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio> // Para la función rename()
#include <curl/curl.h>
#include "json.hpp"

using json = nlohmann::json;

struct Paquete {
    std::string nombre;
    std::string descripcion;
    std::string version;
    std::string url;
};

// --- Función para descargar (igual que antes) ---
static size_t write_data(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t written = fwrite(contents, size, nmemb, (FILE *)userp);
    return written;
}

bool descargarArchivo(const std::string& url, const std::string& nombreArchivoLocal) {
    CURL *curl;
    FILE *fp;
    CURLcode res;
    
    curl = curl_easy_init();
    if (!curl) return false;

    fp = fopen(nombreArchivoLocal.c_str(), "wb");
    if (!fp) {
        std::cerr << "Error: No se pudo crear el archivo local." << std::endl;
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    res = curl_easy_perform(curl);
    
    fclose(fp);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK);
}

// --- Función para mostrar la ayuda (ACTUALIZADA) ---
void mostrarAyuda() {
    std::cout << "Uso: ftpack [COMANDO] [PAQUETE]\n\n";
    std::cout << "Comandos:\n";
    std::cout << "  -i <paquete>  Instala el paquete especificado.\n";
    std::cout << "  -update       Actualiza el catálogo de paquetes desde la fuente remota.\n";
    std::cout << "  help          Muestra este mensaje de ayuda y sale.\n\n";
    std::cout << "Ejemplos:\n";
    std::cout << "  ftpack -i htop\n";
    std::cout << "  ftpack -update\n";
    std::cout << "  ftpack help\n";
}

// --- Función principal ---
int main(int argc, char* argv[]) {
    const std::string ruta_json = "/etc/ftpack/packages.json";
    const std::string ruta_config = "/etc/ftpack/config.json";

    // --- Lógica de argumentos de línea de comandos ---
    if (argc < 2) {
        mostrarAyuda();
        return 1;
    }

    std::string comando = argv[1];

    if (comando == "help") {
        mostrarAyuda();
        return 0;
    } else if (comando == "-update") {
        // NUEVA LÓGICA PARA -UPDATE
        std::cout << "🔄 Actualizando el catálogo de paquetes...\n";

        // 1. Leer configuración para obtener la URL remota
        std::ifstream config_file(ruta_config);
        if (!config_file.is_open()) {
            std::cerr << "Error: No se pudo encontrar el archivo de configuración en " << ruta_config << std::endl;
            return 1;
        }
        json config_json = json::parse(config_file);
        std::string remote_url = config_json.value("remote_package_url", "");
        if (remote_url.empty()) {
            std::cerr << "Error: 'remote_package_url' no está definido en " << ruta_config << std::endl;
            return 1;
        }

        // 2. Descargar el nuevo catálogo a un archivo temporal
        std::string temp_file = "/tmp/packages_new.json";
        if (!descargarArchivo(remote_url, temp_file)) {
            std::cerr << "Error: Falló la descarga del nuevo catálogo." << std::endl;
            return 1;
        }
        std::cout << "✅ Nuevo catálogo descargado.\n";

        // 3. Validar que el JSON descargado es válido
        try {
            json test_parse = json::parse(std::ifstream(temp_file));
        } catch (json::parse_error& e) {
            std::cerr << "Error: El archivo descargado no tiene un formato JSON válido: " << e.what() << std::endl;
            std::remove(temp_file.c_str()); // Limpiar el archivo temporal corrupto
            return 1;
        }
        std::cout << "✅ Catálogo validado.\n";

        // 4. Reemplazar el antiguo por el nuevo
        if (std::rename(temp_file.c_str(), ruta_json.c_str()) != 0) {
            std::cerr << "Error: No se pudo reemplazar el catálogo antiguo. ¿Necesitas permisos de sudo?" << std::endl;
            std::remove(temp_file.c_str()); // Limpiar el archivo temporal
            return 1;
        }

        std::cout << "🎉 ¡Catálogo de paquetes actualizado con éxito!\n";
        return 0;

    } else if (comando == "-i") {
        // Lógica de instalación (igual que antes, pero cargando el JSON aquí)
        if (argc < 3) {
            std::cerr << "Error: El comando '-i' requiere el nombre de un paquete.\n";
            std::cerr << "Usa 'ftpack help' para más información.\n";
            return 1;
        }
        std::string nombre_paquete_buscado = argv[2];

        std::ifstream archivo_json(ruta_json);
        if (!archivo_json.is_open()) {
            std::cerr << "Error fatal: No se pudo encontrar el catálogo de paquetes en " << ruta_json << std::endl;
            std::cerr << "Intenta ejecutar 'ftpack -update' primero." << std::endl;
            return 1;
        }

        json datos_json;
        try {
            datos_json = json::parse(archivo_json);
        } catch (json::parse_error& e) {
            std::cerr << "Error fatal: El catálogo de paquetes tiene un formato JSON inválido." << std::endl;
            return 1;
        }
        
        // ... (el resto del código de instalación es el mismo) ...
        Paquete paquete_encontrado;
        bool encontrado = false;
        for (const auto& item : datos_json) {
            if (item.value("nombre", "") == nombre_paquete_buscado) {
                paquete_encontrado = { item.value("nombre", ""), item.value("descripcion", ""), item.value("version", ""), item.value("url", "") };
                encontrado = true;
                break;
            }
        }

        if (!encontrado) {
            std::cerr << "Error: El paquete '" << nombre_paquete_buscado << "' no fue encontrado en el catálogo.\n";
            return 1;
        }

        std::string nombre_archivo = paquete_encontrado.nombre + "_" + paquete_encontrado.version + ".deb";
        std::cout << "Preparando para descargar " << paquete_encontrado.nombre << "...\n";

        if (descargarArchivo(paquete_encontrado.url, nombre_archivo)) {
            std::cout << "Descarga completada. Iniciando instalación...\n";
            std::string comando_instalacion = "sudo dpkg -i " + nombre_archivo;
            int resultado = system(comando_instalacion.c_str());
            
            if (resultado == 0) {
                std::cout << "¡Instalación finalizada con éxito!\n";
            } else {
                std::cerr << "La instalación falló. Puede que necesites ejecutar 'sudo apt-get install -f' para corregir dependencias.\n";
            }
        } else {
            std::cerr << "La descarga falló. No se pudo instalar el paquete.\n";
            return 1;
        }

    } else {
        std::cerr << "Comando desconocido: '" << comando << "'\n";
        mostrarAyuda();
        return 1;
    }

    return 0;
}