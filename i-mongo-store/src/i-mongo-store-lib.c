#include "i-mongo-store-lib.h"
#include <stdlib.h>

i_mongo_store_config *leer_config_i_mongo_store(char *path)
{
	t_config *config_aux = config_create(path);
	i_mongo_store_config *config_i_mongo_store_aux = malloc(sizeof(i_mongo_store_config));

	config_i_mongo_store_aux->PUNTO_MONTAJE = config_get_string_value(config_aux, "PUNTO_MONTAJE");
	config_i_mongo_store_aux->PUERTO = config_get_int_value(config_aux, "PUERTO");
	config_i_mongo_store_aux->TIEMPO_SINCRONIZACION = config_get_int_value(config_aux, "TIEMPO_SINCRONIZACION");
	config_i_mongo_store_aux->IP_DISCORDIADOR = config_get_string_value(config_aux, "IP_DISCORDIADOR");
	config_i_mongo_store_aux->PUERTO_DISCORDIADOR = config_get_int_value(config_aux, "PUERTO_SABOTAJE_DISCORDIADOR");
	return config_i_mongo_store_aux;
	// No hace falta liberar memoria
}

t_log *iniciar_logger_i_mongo_store(char *path)
{

	t_log *log_aux = log_create(path, "I-MONGO-STORE", 1, LOG_LEVEL_INFO);
	return log_aux;
}

void *esperar_conexion(int puerto)
{
	t_list *hilos = list_create();
	int socket_escucha = iniciar_servidor_i_mongo_store(puerto);
	int socket_i_mongo_store;
	log_info(logger_i_mongo_store, "SERVIDOR LEVANTADO! ESCUCHANDO EN %i", puerto);
	struct sockaddr cliente;
	socklen_t len = sizeof(cliente);
	do
	{
		socket_i_mongo_store = accept(socket_escucha, &cliente, &len);
		if (socket_i_mongo_store > 0)
		{
			log_info(logger_i_mongo_store, "NUEVA CONEXIÓN!");
			crear_hilo_para_manejar_suscripciones(hilos, socket_i_mongo_store);
		}
		else
		{
			log_error(logger_i_mongo_store, "ERROR ACEPTANDO CONEXIONES: %s", strerror(errno));
		}
	} while (1);
}

int iniciar_servidor_i_mongo_store(int puerto)
{
	int socket_v;
	int val = 1;
	struct sockaddr_in servaddr;

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(puerto);

	socket_v = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_v < 0)
	{
		char *error = strerror(errno);
		perror(error);
		free(error);
		return EXIT_FAILURE;
	}

	setsockopt(socket_v, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	if (bind(socket_v, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		return EXIT_FAILURE;
	}
	if (listen(socket_v, MAX_CLIENTS) < 0)
	{
		return EXIT_FAILURE;
	}

	return socket_v;
}

void crear_hilo_para_manejar_suscripciones(t_list *lista_hilos, int socket)
{
	int *socket_hilo = malloc(sizeof(int));
	*socket_hilo = socket;
	pthread_t hilo_conectado;
	pthread_create(&hilo_conectado, NULL, (void *)manejar_suscripciones_i_mongo_store, socket_hilo);
	list_add(lista_hilos, &hilo_conectado);
}

//Socket envio es para enviar la respuesta, de momento no lo uso
void *manejar_suscripciones_i_mongo_store(int *socket_envio)
{
	log_info(logger_i_mongo_store, "Se creó el hilo correctamente");
	t_paquete *paquete = recibir_paquete(*socket_envio);
	void *stream = paquete->stream;
	op_code codigo_respuesta;
	uint32_t tamanio_respuesta;
	void *stream_respuesta;
	switch (paquete->codigo_operacion)
	{
	case GENERAR_RECURSO:
	{
		operacion_recurso op_c_rec = deserializar_operacion_recurso(stream);
		op_c_rec.recurso = a_mayusc_primera_letra(op_c_rec.recurso);
		char *escritura = NULL;
		if (op_c_rec.cantidad > 0)
			;
		escritura = malloc(op_c_rec.cantidad + 1);
		char *archivo = malloc(strlen(carpeta_files) + strlen(".ims") + strlen(op_c_rec.recurso) + 1);
		memcpy(archivo, carpeta_files, strlen(carpeta_files) + 1);
		strcat(archivo, op_c_rec.recurso);
		strcat(archivo, ".ims");
		for (int i = 0; i < op_c_rec.cantidad; i++)
		{
			escritura[i] = op_c_rec.recurso[0];
		}
		escribir_archivo(archivo, escritura,RECURSO);
		free(archivo);
		free(escritura);
		codigo_respuesta = RESPUESTA_GENERAR_RECURSO;
		stream_respuesta = serializar_respuesta_ok_fail(RESPUESTA_OK);
		tamanio_respuesta = sizeof(respuesta_ok_fail);
	}
	break;

	case CONSUMIR_RECURSO:
	{
		operacion_recurso op_c_rec = deserializar_operacion_recurso(stream);
		op_c_rec.recurso = a_mayusc_primera_letra(op_c_rec.recurso);
		char *escritura = NULL;
		if (op_c_rec.cantidad > 0)
			;
		escritura = malloc(op_c_rec.cantidad + 1);
		char *archivo = malloc(strlen(carpeta_files) + strlen(".ims") + strlen(op_c_rec.recurso) + 1);
		memcpy(archivo, carpeta_files, strlen(carpeta_files) + 1);
		strcat(archivo, op_c_rec.recurso);
		strcat(archivo, ".ims");
		for (int i = 0; i < op_c_rec.cantidad; i++)
		{
			escritura[i] = op_c_rec.recurso[0];
		}
		quitar_de_archivo(archivo, escritura);
		free(archivo);
		free(escritura);
		codigo_respuesta = RESPUESTA_CONSUMIR_RECURSO;
		stream_respuesta = serializar_respuesta_ok_fail(RESPUESTA_OK);
		tamanio_respuesta = sizeof(respuesta_ok_fail);
	}
	break;

	case VACIAR_RECURSO:
	{
		char *rec = deserializar_recurso(stream);
		rec = a_mayusc_primera_letra(rec);
		char *archivo = malloc(strlen(carpeta_files) + strlen(".ims") + strlen(rec) + 1);
		memcpy(archivo, carpeta_files, strlen(carpeta_files) + 1);
		strcat(archivo, rec);
		strcat(archivo, ".ims");
		borrar_archivo(archivo);
		free(archivo);
		codigo_respuesta = RESPUESTA_VACIAR_RECURSO;
		stream_respuesta = serializar_respuesta_ok_fail(RESPUESTA_OK);
		tamanio_respuesta = sizeof(respuesta_ok_fail);
	}
	break;

	case REGISTRAR_MOVIMIENTO:
	{
		movimiento_tripulante trip_y_movs = deserializar_movimiento_tripulante(stream);
		char *posX_vieja = itoa_propio(trip_y_movs.pos_x_vieja);
		char *posY_vieja = itoa_propio(trip_y_movs.pos_y_vieja);
		char *posX_nueva = itoa_propio(trip_y_movs.pos_x_nueva);
		char *posY_nueva = itoa_propio(trip_y_movs.pos_y_nueva);
		char *tripulante_id = itoa_propio(trip_y_movs.tid);
		char *frase1 = "El tripulante: ";
		char *frase2 = " se movió desde la posición X: ";
		char *frase3 = " e Y: ";
		char *frase4 = " a la posición nueva X: ";
		char *frase5 = " y nueva Y: ";
		char *frase = malloc(strlen(posX_vieja) + strlen(posY_vieja) + strlen(posX_nueva) + strlen(posY_nueva) + strlen(tripulante_id) + strlen(frase1) + strlen(frase2) + strlen(frase3) + strlen(frase4) + strlen(frase5) + 1);
		frase[0] = '\0'; //creo que con esto me evito el memcpy
		strcat(frase, frase1);
		strcat(frase, tripulante_id);
		strcat(frase, frase2);
		strcat(frase, posX_vieja);
		strcat(frase, frase3);
		strcat(frase, posY_vieja);
		strcat(frase, frase4);
		strcat(frase, posX_nueva);
		strcat(frase, frase5);
		strcat(frase, posY_nueva);
		char *archivo = malloc(strlen(carpeta_bitacoras) + strlen(".ims") + strlen("Tripulante") + strlen(tripulante_id) + 1);
		memcpy(archivo, carpeta_bitacoras, strlen(carpeta_bitacoras) + 1);
		strcat(archivo, "Tripulante");
		strcat(archivo, tripulante_id);
		strcat(archivo, ".ims");
		escribir_archivo(archivo, frase, BITACORA);
		free(tripulante_id);
		free(posX_vieja);
		free(posY_vieja);
		free(posX_nueva);
		free(posY_nueva);
		free(frase);
		free(archivo);
		codigo_respuesta = RESPUESTA_REGISTRAR_MOVIMIENTO;
		stream_respuesta = serializar_respuesta_ok_fail(RESPUESTA_OK);
		tamanio_respuesta = sizeof(respuesta_ok_fail);
	}
	break;

	case REGISTRAR_COMIENZO_TAREA:
	{
		tripulante_con_tarea trip_c_tarea = deserializar_tripulante_con_tarea(stream);
		char *frase1 = "El tripulante: ";
		char *frase2 = " ha comenzado con la tarea: ";
		char *tripulante_id = itoa_propio(trip_c_tarea.tid);
		char *frase = malloc(strlen(frase1) + strlen(frase2) + strlen(tripulante_id) + strlen(trip_c_tarea.tarea) + 1);
		frase[0] = '\0';
		strcat(frase, frase1);
		strcat(frase, tripulante_id);
		strcat(frase, frase2);
		strcat(frase, trip_c_tarea.tarea);
		char *archivo = malloc(strlen(carpeta_bitacoras) + strlen(".ims") + strlen("Tripulante") + strlen(tripulante_id) + 1);
		memcpy(archivo, carpeta_bitacoras, strlen(carpeta_bitacoras) + 1);
		strcat(archivo, "Tripulante");
		strcat(archivo, tripulante_id);
		strcat(archivo, ".ims");
		escribir_archivo(archivo, frase, BITACORA);
		free(trip_c_tarea.tarea);
		free(tripulante_id);
		free(frase);
		free(archivo);
		codigo_respuesta = RESPUESTA_REGISTRAR_COMIENZO_TAREA;
		stream_respuesta = serializar_respuesta_ok_fail(RESPUESTA_OK);
		tamanio_respuesta = sizeof(respuesta_ok_fail);
	}
	break;

	case REGISTRAR_FIN_TAREA:
	{
		tripulante_con_tarea trip_c_tarea = deserializar_tripulante_con_tarea(stream);
		char *frase1 = "El tripulante: ";
		char *frase2 = " ha finalizado la tarea: ";
		char *tripulante_id = itoa_propio(trip_c_tarea.tid);
		char *frase = malloc(strlen(frase1) + strlen(frase2) + strlen(tripulante_id) + strlen(trip_c_tarea.tarea) + 1);
		frase[0] = '\0';
		strcat(frase, frase1);
		strcat(frase, tripulante_id);
		strcat(frase, frase2);
		strcat(frase, trip_c_tarea.tarea);
		char *archivo = malloc(strlen(carpeta_bitacoras) + strlen(".ims") + strlen("Tripulante") + strlen(tripulante_id) + 1);
		memcpy(archivo, carpeta_bitacoras, strlen(carpeta_bitacoras) + 1);
		strcat(archivo, "Tripulante");
		strcat(archivo, tripulante_id);
		strcat(archivo, ".ims");
		escribir_archivo(archivo, frase, BITACORA);
		free(trip_c_tarea.tarea);
		free(tripulante_id);
		free(frase);
		free(archivo);
		codigo_respuesta = RESPUESTA_REGISTRAR_FIN_TAREA;
		stream_respuesta = serializar_respuesta_ok_fail(RESPUESTA_OK);
		tamanio_respuesta = sizeof(respuesta_ok_fail);
	}
	break;

	case REGISTRAR_MOVIMIENTO_A_SABOTAJE:
	{
		movimiento_tripulante trip_y_movs = deserializar_movimiento_tripulante(stream);
		char *posX_vieja = itoa_propio(trip_y_movs.pos_x_vieja);
		char *posY_vieja = itoa_propio(trip_y_movs.pos_y_vieja);
		char *posX_nueva = itoa_propio(trip_y_movs.pos_x_nueva);
		char *posY_nueva = itoa_propio(trip_y_movs.pos_y_nueva);
		char *tripulante_id = itoa_propio(trip_y_movs.tid);
		char *frase1 = "El tripulante: ";
		char *frase2 = " se movió desde la posición X: ";
		char *frase3 = " e Y: ";
		char *frase4 = " a la posición nueva X: ";
		char *frase5 = " y nueva Y: ";
		char *frase6 = " para solucionar el sabotaje existente";
		char *frase = malloc(strlen(posX_vieja) + strlen(posY_vieja) + strlen(posX_nueva) + strlen(posY_nueva) + strlen(tripulante_id) + strlen(frase1) + strlen(frase2) + strlen(frase3) + strlen(frase4) + strlen(frase5) + strlen(frase6) + 1);
		frase[0] = '\0'; //creo que con esto me evito el memcpy
		strcat(frase, frase1);
		strcat(frase, tripulante_id);
		strcat(frase, frase2);
		strcat(frase, posX_vieja);
		strcat(frase, frase3);
		strcat(frase, posY_vieja);
		strcat(frase, frase4);
		strcat(frase, posX_nueva);
		strcat(frase, frase5);
		strcat(frase, posY_nueva);
		strcat(frase, frase6);
		char *archivo = malloc(strlen(carpeta_bitacoras) + strlen(".ims") + strlen("Tripulante") + strlen(tripulante_id) + 1);
		memcpy(archivo, carpeta_bitacoras, strlen(carpeta_bitacoras) + 1);
		strcat(archivo, "Tripulante");
		strcat(archivo, tripulante_id);
		strcat(archivo, ".ims");
		escribir_archivo(archivo, frase, BITACORA);
		free(tripulante_id);
		free(posX_vieja);
		free(posY_vieja);
		free(posX_nueva);
		free(posY_nueva);
		free(frase);
		free(archivo);
		codigo_respuesta = RESPUESTA_REGISTRAR_MOVIMIENTO_A_SABOTAJE;
		stream_respuesta = serializar_respuesta_ok_fail(RESPUESTA_OK);
		tamanio_respuesta = sizeof(respuesta_ok_fail);
	}
	break;

	case REGISTRAR_SABOTAJE_RESUELTO:
	{
		uint32_t tid;
		memcpy(&tid, stream, sizeof(uint32_t));
		char *frase1 = "El sabotaje ha sido resuelto por el tripulante: ";
		char *tripulante_id = itoa_propio(tid);
		char *frase = malloc(strlen(frase1) + strlen(tripulante_id) + 1);
		frase[0] = '\0';
		strcat(frase, frase1);
		strcat(frase, tripulante_id);
		char *archivo = malloc(strlen(carpeta_bitacoras) + strlen(".ims") + strlen("Tripulante") + strlen(tripulante_id) + 1);
		memcpy(archivo, carpeta_bitacoras, strlen(carpeta_bitacoras) + 1);
		strcat(archivo, "Tripulante");
		strcat(archivo, tripulante_id);
		strcat(archivo, ".ims");
		escribir_archivo(archivo, frase, BITACORA);
		free(tripulante_id);
		free(frase);
		free(archivo);
		codigo_respuesta = RESPUESTA_REGISTRAR_SABOTAJE_RESUELTO;
		stream_respuesta = serializar_respuesta_ok_fail(RESPUESTA_OK);
		tamanio_respuesta = sizeof(respuesta_ok_fail);
	}
	break;

	case OBTENER_BITACORA:
	{
		uint32_t tid;
		memcpy(&tid, stream, sizeof(uint32_t));
		char *tripulante_id = itoa_propio(tid);
		char *archivo = malloc(strlen(carpeta_bitacoras) + strlen(".ims") + strlen("Tripulante") + strlen(tripulante_id) + 1);
		memcpy(archivo, carpeta_bitacoras, strlen(carpeta_bitacoras) + 1);
		strcat(archivo, "Tripulante");
		strcat(archivo, tripulante_id);
		strcat(archivo, ".ims");
		char *contenido = todo_el_archivo(archivo);
		uint32_t tamanio = strlen(contenido) + 1;
		tamanio_respuesta = tamanio + sizeof(uint32_t);
		stream_respuesta = malloc(tamanio + sizeof(uint32_t));
		memcpy(stream_respuesta, &tamanio, sizeof(uint32_t));
		memcpy(stream_respuesta + sizeof(uint32_t), contenido, tamanio);
		codigo_respuesta = RESPUESTA_OBTENER_BITACORA;
		free(tripulante_id);
		free(archivo);
	}

	default:
		//no me rompas las bolainas
		break;
	}
	t_paquete *paquete_respuesta = malloc(sizeof(t_paquete));
	paquete_respuesta->codigo_operacion = codigo_respuesta;
	paquete_respuesta->size = tamanio_respuesta;
	paquete_respuesta->stream = stream_respuesta;
	void *a_enviar = serializar_paquete(paquete_respuesta, paquete_respuesta->size + sizeof(op_code) + sizeof(size_t));
	send(*socket_envio, a_enviar, paquete_respuesta->size + sizeof(op_code) + sizeof(size_t), 0);
	liberar_paquete(paquete);
	free(a_enviar);
	close(*socket_envio);
	free(socket_envio);
	return NULL;
}

void inicializar_filesystem(uint32_t block_size, uint32_t block_amount)
{
	crear_directorio(i_mongo_store_configuracion->PUNTO_MONTAJE);
	inicializar_superbloque(block_size, block_amount);
	inicializar_blocks(block_size, block_amount);
	crear_directorio(carpeta_files);
	crear_directorio(carpeta_bitacoras);
	log_info(logger_i_mongo_store, "FILESYSTEM INICIALIZADO DE 0 CON EXITO");
	uint32_t *tamanios = malloc(2 * sizeof(uint32_t));
	tamanios[0] = 2 * sizeof(uint32_t) + block_amount;
	tamanios[1] = block_amount * block_size;
	pthread_create(&hilo_sincronizacion, NULL, sincronizar, (void *)tamanios);
}

void inicializar_superbloque(uint32_t block_size, uint32_t block_amount)
{
	int fd = open(ruta_superbloque, O_CREAT | O_RDWR, (mode_t)0777);
	if (fd == -1)
	{
		no_pude_abrir_archivo(ruta_superbloque);
		exit(-1);
	}
	ftruncate(fd, 2 * sizeof(uint32_t) + block_amount);
	superbloque = mmap(NULL, 2 * sizeof(uint32_t) + block_amount, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	if (superbloque == MAP_FAILED)
	{
		no_pude_mapear_archivo(ruta_superbloque);
		exit(-1);
	}
	memcpy(superbloque, &block_size, sizeof(uint32_t));
	memcpy(superbloque + sizeof(uint32_t), &block_amount, sizeof(uint32_t));
	char vector_disponibilidad[block_amount];
	for (int i = 0; i < block_amount; i++)
	{
		vector_disponibilidad[i] = '0';
	}
	t_bitarray* bitarray = bitarray_create_with_mode(vector_disponibilidad,block_amount,MSB_FIRST);
	memcpy(superbloque + 2* sizeof(uint32_t), bitarray, sizeof(t_bitarray));
	msync(superbloque, 2 * sizeof(uint32_t) + sizeof(bitarray), 0);
	struct stat *algo = malloc(sizeof(struct stat));
	int resultado = fstat(fd, algo);
	if (resultado == -1)
	{
		log_error(logger_i_mongo_store, "no anduvo fstat");
	}
	else
	{
		log_info(logger_i_mongo_store, "cant bloques: %d, tam bloques: %d, ", algo->st_blocks, algo->st_blksize);
	}
}

void inicializar_blocks(uint32_t block_size, uint32_t block_amount)
{
	int fd = open(ruta_blocks, O_CREAT | O_RDWR, (mode_t)0777);
	if (fd == -1)
	{
		no_pude_abrir_archivo(ruta_blocks);
	}
	fallocate(fd, 0, 0, block_size * block_amount);
	blocks = mmap(NULL, block_size * block_amount, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	if (blocks == MAP_FAILED)
	{
		no_pude_mapear_archivo(ruta_blocks);
		exit(-1);
	}
}

void crear_directorio(char *carpeta)
{
	int res = mkdir(carpeta, (mode_t)0777);
	if (res == -1)
	{
		log_error(logger_i_mongo_store, "No pude crear la carpeta %s", carpeta);
		exit(-1);
	}
}

void no_pude_abrir_archivo(char *ruta_archivo)
{
	log_error(logger_i_mongo_store, "No se pudo abrir el archivo %s", ruta_archivo);
}

void no_pude_mapear_archivo(char *ruta_archivo)
{
	log_error(logger_i_mongo_store, "No se pudo mapear el archivo %s", ruta_archivo);
}

char *a_mayusc_primera_letra(char *palabra)
{
	for (int i = 0; palabra[i]; i++)
	{
		if (i == 0)
		{
			palabra[i] = toupper(palabra[i]);
		}
		else
		{
			palabra[i] = tolower(palabra[i]);
		}
	}
	return palabra;
}

bool existe_recurso(char *recurso)
{
	char *archivo = malloc(strlen(carpeta_files) + strlen(recurso) + strlen(".ims") + 1);
	memcpy(archivo, carpeta_files, strlen(carpeta_files) + 1);
	strcat(archivo, recurso);
	strcat(archivo, ".ims");

	DIR *dir = opendir(archivo);
	if (dir)
	{
		closedir(dir);
		return 1;
	}
	else if (ENOENT == errno)
	{
		log_error(logger_i_mongo_store, "La ruta %s se quiso abrir pero no existe", archivo);
		return 0;
	}
	else
	{
		log_error(logger_i_mongo_store, "La ruta %s se quiso abrir. Existe pero hubo otro fallo", archivo);
		return 1;
	}
	return 0;
}

void *sincronizar(void *tamanio)
{ //como si fuera un array
	while (1)
	{
		uint32_t *tamanios = (uint32_t *)tamanio;
		msync(superbloque, tamanios[0], 0);
		msync(blocks, tamanios[1], 0);
		sleep(i_mongo_store_configuracion->TIEMPO_SINCRONIZACION);
	}
}

char *itoa_propio(uint32_t entero)
{
	int aux;
	int digitos;
	char convertido;
	digitos = 0;
	aux = entero;
	if (entero == 0)
	{
		char *unCero = malloc(2);
		unCero[0] = entero + '0';
		unCero[1] = '\0';
		return unCero;
	}
	while (aux > 0)
	{
		aux = aux / 10;
		digitos += 1;
	}
	char *cadena = malloc(digitos + 1);
	cadena[digitos] = '\0';
	while (entero)
	{
		convertido = entero % 10 + '0';
		cadena[digitos - 1] = convertido;
		digitos--;
		entero = entero / 10;
	}
	return cadena;
}

void escribir_archivo(char *ruta, char *contenido, tipo_archivo tipo)
{
	if (tipo == BITACORA)
	{
		strcat(contenido, "\n");
	}
	FILE *archivo;
	archivo = fopen(ruta, "r+");
	if (archivo == NULL && errno == ENOENT)
	{
		crear_archivo_metadata(ruta, tipo, contenido[0]);
		log_info(logger_i_mongo_store, "Se creo el archivo %s", ruta);
	}
	else if (archivo == NULL && errno != ENOENT)
	{
		log_error(logger_i_mongo_store, "El archivo %s existe, pero no se pudo abrir", ruta);
	}
	t_config *llave_valor = config_create(ruta);
	int cant_bloques = config_get_int_value(llave_valor, "BLOCK_COUNT");
	int longitud_restante = strlen(contenido);
	int caracteres_llenados = 0;
	//TODO: CONSEGUIR BLOCK_SIZE
	int block_size = 1024; //hardcodeo
	if (cant_bloques != 0)
	{
		int nro_ultimo_bloque = conseguir_ultimo_bloque(llave_valor, cant_bloques);
		char *ultimo_bloque = malloc(block_size);
		memcpy(ultimo_bloque, blocks + (nro_ultimo_bloque * block_size), block_size);
		int ultima_posicion_escrita = encontrar_anterior_barra_cero(ultimo_bloque, block_size); //Devuelve la posición del ultimo caracter anterior al barra cero
		for (int j = 1; (j + ultima_posicion_escrita) < block_size && longitud_restante > 0; j++)
		{ //Arranco desde el \0
			ultimo_bloque[j + ultima_posicion_escrita] = contenido[j - 1];
			longitud_restante--;
			caracteres_llenados++;
		}
		log_info(logger_i_mongo_store, "Se han llenado %d caracteres en el bloque %d, asignados al archivo %s",
				 caracteres_llenados, nro_ultimo_bloque, ruta);
		if (ultima_posicion_escrita + caracteres_llenados + 1 < block_size)
		{ //Me sobró bloque, pongo barra cero
			ultimo_bloque[ultima_posicion_escrita + caracteres_llenados + 1] = '\0';
		}
		memcpy(blocks + (nro_ultimo_bloque * block_size), ultimo_bloque, block_size);
	}
	while (longitud_restante > 0)
	{
		/* 1- Hallar bloque libre (leer superbloque)
			2- Reservarlo (escribir superbloque, escribir files/bitacora .ims) alterando block_count y blocks, MD5
			3- Memcpy del blocks
			4- Asignar cada byte del bloque al caracter correspondiente, hasta llenar el bloque o agotar los caracteres,
			actualizando longitud_restante y caracteres_llenados
			5- Si sobró espacio de bloque, colocar el \0
			6- Memcpy al blocks
			7- Reiterar si longitud_restante no es 0
			 */
	}
}

void quitar_de_archivo(char *archivo, char *contenido)
{
	//TODO
}

void borrar_archivo(char *archivo)
{
	//TODO
}

char *todo_el_archivo(char *archivo)
{
	//TODO
	return "tu vieja";
}

void crear_archivo_metadata(char *ruta, tipo_archivo tipo, char caracter_llenado)
{
	FILE *archivo = fopen(ruta, "w+");
	fclose(archivo);
	t_config *f = config_create(ruta);
	config_set_value(f, "SIZE", 0);
	config_set_value(f, "BLOCK_COUNT", 0);
	config_set_value(f, "BLOCKS", "[]");
	char* car_llenado = malloc(sizeof(char));
	*car_llenado = caracter_llenado;
	if (tipo == RECURSO)
	{
		config_set_value(f, "CARACTER_LLENADO", car_llenado);
		config_set_value(f, "MD5", "Noseparaquesirvestodavia");
	}
	config_save_in_file(f, ruta);
	config_destroy(f);
	free(car_llenado);
}

int conseguir_ultimo_bloque(t_config *llave_valor, int cant_bloques)
{
	char *bloques = config_get_string_value(llave_valor, "BLOCKS");
	int longitud_bloques = strlen(bloques) + 1;
	char *bloques_aux = malloc(longitud_bloques - 2);
	for (int i = 1; i < longitud_bloques - 2; i++)
	{
		bloques_aux[i - 1] = bloques[i];
	}
	bloques_aux[longitud_bloques - 3] = '\0';
	char **bloques_spliteados = string_split(bloques_aux, ",");
	return atoi(bloques_spliteados[cant_bloques - 1]);
}

int encontrar_anterior_barra_cero(char* ultimo_bloque, int block_size){
	return 0; //hardcodeo
}

int get_block_size(){
	uint32_t bs;
	memcpy(&bs,superbloque,sizeof(uint32_t));
	return bs;
}

int get_block_amount(){
	uint32_t ba;
	memcpy(&ba, superbloque+sizeof(uint32_t),sizeof(uint32_t));
	return ba;
}

int get_primer_bloque_libre(){
	int cant_bloques = get_block_amount();
	char* bitarray = malloc(cant_bloques);
	t_bitarray* bit_array = malloc(sizeof(t_bitarray));
	bit_array->bitarray = bitarray;
	memcpy(bit_array, superbloque + 2*sizeof(uint32_t),sizeof(t_bitarray));
	bool ocupado = 1;
	int i;
	for(i = 0; i<bitarray_get_max_bit(bit_array) || !ocupado;i++){
		ocupado = bitarray_test_bit(bit_array,i);
	}
	free(bitarray);
	free(bit_array);
	if(ocupado == 0){
		return i;
	} else{
		return -1;
	}
}

void liberar_bloque(int numero_bloque){
	int cant_bloques = get_block_amount();
	char* bitarray = malloc(cant_bloques);
	t_bitarray* bit_array = malloc(sizeof(t_bitarray));
	bit_array->bitarray = bitarray;
	memcpy(bit_array, superbloque + 2*sizeof(uint32_t),sizeof(t_bitarray));
	bitarray_clean_bit(bit_array, numero_bloque);
	free(bit_array);
	free(bitarray);
}

void ocupar_bloque(int numero_bloque){
	int cant_bloques = get_block_amount();
	char* bitarray = malloc(cant_bloques);
	t_bitarray* bit_array = malloc(sizeof(t_bitarray));
	bit_array->bitarray = bitarray;
	memcpy(bit_array, superbloque + 2*sizeof(uint32_t),sizeof(t_bitarray));
	bitarray_set_bit(bit_array, numero_bloque);
	free(bit_array);
	free(bitarray);
}