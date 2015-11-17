Instalación Raspbian en Raspberry Pi
--------------------------------------

1 Instalar el sistema operativo en nuestra raspberry Pi:

```sh
$diskutil list
$diskutil unmountDisk /dev/disk4
$sudo dd bs=1m if=image.img of=/dev/rdisk4
```

2 Instalar WiringPi

```sh
$git clone git://git.drogon.net/wiringPi
$cd wiringPi
$./build
```
En caso de funcionar la conexión con Git, lo mejor es descargarse el fichero máster como Zip de la URL: https://github.com/WiringPi/WiringPi y copiarlo en la Raspberry Pi. Una vez extraído, instalar con ./build

3 Preparar el entorno para la comunicación de datos con el sensor I2C

```sh
$sudo raspi-config
$sudo apt-get install i2c-tools
$i2cdetect –y 1
```

Si todo está configurado correctamente, tras ejecutar el último comando se debe obtener un 27 en la columna 7, fila 20.

4 Instalar las librerías del display táctil

Descargar el fichero máster de https://github.com/4dsystems/ViSi-Genie-RaspPi-Library copiar en la Raspberry Pi y seguir los pasos del fichero de instalación contenido en el máster (ver código más abajo).

```sh
$cd ViSi-Genie-RaspPi-Library-master
$sudo make install
```

Instalar el entorno de desarrollo de la pantallas táctiles (bajo un sistema operativo Windows) desde la página de 4D-Systems http://www.4dsystems.com.au/product/4D_Workshop_4_IDE/
