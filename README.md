Visas užduotis atlikau su STM32CubeIDE. Pasirinkus turimą DISCOVERY plokštę, sujungimų kodas yra automatiškai sugeneruojamas, taip pat galima keisti sujungimus ir jų parametrus per grafinį interfeisą.
# 2 Užduotis
***Sukurkite sekundės periodo laikmatį. Jam veikiant, šviestukas periodiškai įjungiamas 50 ms ir išjungiamas 950 ms***
Galima būtų suprasti kad laikmatis yra timer arba clock ir reikia sukonfiguruoti vieną iš hardware timerių kad jis "tiksėtų" kartą į sekundę, tačiau pagal užduoties pobūdį tai atrodo nelogiška. Sekundės periodo laikmatį suprantu kaip: "laikas tarp šviestuko įsijungimų turi būti 1s".  

Kadangi užduoties puktai nedaug skiriasi tarpusavyje, visus idėjau į vieną projektą ir padariau, kad kiekvienas metodas junginėtų skirtingą šviestuką. Visuose punktuose pradedu nuo išjungto šviestuko pirmas 950ms ir tada paskutines 50ms įjungiu - rezultatas tas pats nes visas procesas kartojasi.
## 2.1 Rezultatą pasiekite taikydami: HAL_Delay()
Pirmas metodas ganėtinai paprastas ir aiškus - šviestukas įjungiamas, 50ms laukimas, tada išjungiamas, laukimas 950ms ir vėl kartojasi nuo pradžių. GPIO port'ų manipuliavimui naudojame Hardware Abstraction Layer (HAL) funkcijas.

```c
	// Red LED
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
	HAL_Delay(950);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
	HAL_Delay(50);
```
## 2.2 Rezultatą pasiekite taikydami: SysTick_Handler()
SysTick_Handler() funkcija yra automatiškai iškviečiama kiekvieną "ticką" kas yra 1ms. Papildę šią funkciją stm32f4xx_it.c faile savo kodu, galime skaičiuoti kiek praėjo laiko ir kas sekundę įjungti ar išjungti LED'ą.
```c
static volatile uint32_t ms_ticks = 0;

void SysTick_Handler(void)   
{
	ms_ticks++;
	if (ms_ticks == 950){
		// Blue LED
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET);
	}
	else if (ms_ticks == 1000){
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);
		ms_ticks = 0;
	}
  HAL_IncTick();
}
```
## 2.3 Rezultatą pasiekite taikydami: HAL_TIM_PeriodElapsedCallback()
    
Naudodami GUI sukuriame naują timerį. Kad kodas būtų šiek tiek kitoks nei praitame punkte, galime jį sukonfiguruoti kad jis "tiksėtų" kas 50ms. Kadangi base clock yra 168MHZ, tą pasiekiame nustatydami prescaler = 168 ir counter period = 50000, taip pat įjungiame interrupt opciją. Startuojame timerį su:
`HAL_TIM_Base_Start_IT(&htim10);`
ir aprašome analogišką funkciją kaip ir 2.2
```c
static volatile uint8_t ticks_50 = 0;
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  // Because any timer can trigger this, check if it is triggered by htim10
  if (htim == &htim10 )
  {
	  ticks_50++;
	  	if (ticks_50 == 19){
	  		// Green LED
	  		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);

	  	}
	  	else if (ticks_50 == 20){
	  		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);
	  		ticks_50 = 0;
	  	}
  }
}
```
Video demonstracija:
https://user-images.githubusercontent.com/47836357/148704352-c400b2b7-6392-49d6-a012-c114110a9aa0.mp4

Mėlynas ir žalias LED'ai (kontroliuojami timer'ių) laikui bėgant lieka susisinchronizavę, tačiau raudonas (įjungiamas su HAL_DELAY() ) išsderina. Taip yra todėl, nes timer'iai visada "tiksi" vienodu tempu. Raudono LED'o junginėjimas main loop'e neatsižvelgia į tai, kad LED'o įjungimas ar išjungimas (ir galimai pats HAL_DELAY() iššaukimas) užtrunka tam tikrą sekundės ųdalį. Dėl to laikui bėgant šis neatitikimas pereina į matoma nukrypimą nuo kitų LED'ų.


# 3 Užduotis 
***Pademonstruokite analoginio signalo įtampos (0 – 3,3 V) DAC išėjime vertės priklausomybę nuo User mygtuko paspaudimo skaičiaus***


## Arduino ADC
Kad parodyti DAC išėjimo įtampą panaudojau Arduino Uno. Nuskaitome analog input piną ir konvertuojame iš digital vertės į analoginę dešimtainę vertę. Rezultatą nusiunčiame per serial port'ą.
```c
int analog_pin = A5;
int val = 0;
float val_V = 0.0;
void setup() {
  Serial.begin(9600);
  pinMode(analog_pin, INPUT);
}

void loop() {
  val = analogRead(analog_pin);
  val_V = val * (5.0 / 1024.0);
  Serial.println("Measured voltage is " + (String)val_V +" V");
  delay(200);
}
```
iš nuskaitomos digital vertės į analog galime paversti pagal formulę: <img src="https://render.githubusercontent.com/render/math?math=V_{in} = Digital\ Value \times \frac{V_{ref}} {2^N}">
N  - ADC bitų skaičius, šiuo atvėju 10. Pagal ATMega328P datasheet 1 atimti nereikia.

## Pagrindinis kodas
Pirmiausia nustatome mygtuko jungtį kaip "External Interrupt". Tada aprašome naują funkciją, kuri bus iškviečiama kiekvieną kartą paspaudus mygtuką. Ji skaičiuoja kiek kartų buvo paspaustas mygtukas (taip pat indikuoja paspaudimą įjungiant/išjungiant LEDą).
```c
static volatile uint8_t num_presses = 0;
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
//	Check if the button was pressed
	if(B1_Pin == GPIO_Pin){
		// Toggle LED to indicate button press
		HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
		// Increase pressed button count and roll over if pressed five times
		num_presses++;
		if(num_presses == 5){
			num_presses = 0;
		}
	}
}
```
Arduino nuskaitė STM32 plokštės VDD pin'o vertę (operating voltage) kaip lygiai 3V. Kadangi DAC outputas veikia reliatyviai plokštės reference voltage, maximali DAC išėjimo vertė ir yra 3V. 
3V galime padalinti į 5 lygius žingsnius: 

 - 0V
 - 0.75V
 - 1.5V
 - 2.25V
 - 3V
Analog vertę išreikšti digital skaičiumi galime pagal formulę: <img src="https://render.githubusercontent.com/render/math?math=Digital\ Value = V_{out} \times \frac {2^N}{V_{ref}} ">
Nustatome PA4 pin'ą kaip DAC_OUT1. Main funkcijoje startuojame DAC su:
 `  HAL_DAC_Start(&hdac, DAC_CHANNEL_1);`
Naudodami switch statement'ą pagrindiniam loop'e nustatome išėjimo vertę pagal mygtuo paspaudimų skaičių:

```c
 while (1)
  {
	  //Set DAC value according to number of button presses
	  switch(num_presses){
	  	  case 0 :
			v_dig = 0;
			HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, v_dig);
			break;

	  	  case 1 :
			v_dig = (uint32_t) (0.75 * 4096) / 3.0;
			HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, v_dig);
			break;

	  	  case 2 :
			v_dig = (uint32_t) (1.5 * 4096) / 3.0;
			HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, v_dig);
			break;

	  	  case 3 :
			v_dig = (uint32_t) (2.25 * 4096) / 3.0;
			HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, v_dig);
			break;

	  	  case 4 :
			v_dig = 4095;
			HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, v_dig);
			break;

	  	  default:
			v_dig = 0;
			HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, v_dig);
			break;
	  }
  }
}
```
Video demonstracija:

https://user-images.githubusercontent.com/47836357/148704399-c7c221b0-a816-47a8-8d3d-c2a85ae353e3.mp4



# 4 užduotis
***Susipažinę su garso keitiklio aprašymu, gaukite Chip I.D registro turinį.***
4 ir 5 užduotyje reikia parodyti gautas vertes. Kad tą padaryti, naudodami GUI nustatome USB_OTG režimą kaip Device_Only ir USB_DEVICE klasę kaip Communication Device Class. Dabar galime siųsti duomenis per USB laidą iš plokštės į kompiuterį ir stebėti rezultatą per Serial Monitor.

DISCOVERY plokštės datasheet'e rašoma, kad garso DAC CS43L22 I2C adresas yra 0x94. CS43L22 datasheet'e rašo, kad CHIP_ID registras yra 0x01 lokacijoje ir jo turinys yra 11100 (kas yra 1c šešioliktainėje sistemoje).

Kad įjungti DAC, pirma nustatome RESET pin į 1 (high). Tada naudodami I2C komunikacijos HAL funkcijas, nusiunčiame registro kurio turinį norime nuskaityti adresą iš DAC ir išsaugome gautą atsakymą. Kadangi gauname 1 baitą (kurio 3 LSB yra Chip Revision), right shift'iname per 3 bitus, kad pasiliktume tik Chip i.d. registro turinį. Gautą vertę nusiunčiame per serial port'ą.



 ```c
 uint8_t buffer[35];
 
 while (1)
  {
	  // Need to set the reset pin to high to power up the DAC
	  HAL_GPIO_WritePin(Audio_RST_GPIO_Port, Audio_RST_Pin, GPIO_PIN_SET);
	  buffer[0] = CHIP_ID_REG;
	  // Send read request
	  ret_val = HAL_I2C_Master_Transmit(&hi2c1, CS43L22_ADDR, buffer, 1, HAL_MAX_DELAY);
	  if (ret_val != HAL_OK){
		  strcpy((char*) buffer, "I2C Transmit Error\r\n");
	  }
	  else {
		  // Get the value of specified register
		  ret_val = HAL_I2C_Master_Receive(&hi2c1, CS43L22_ADDR, buffer, 1, HAL_MAX_DELAY);
		  if (ret_val != HAL_OK){
				  strcpy((char*) buffer, "I2C Receive Error\r\n");
			  }
		  else {
			  // Right shift by 3 to get only the chip i.d.
			  val = (int16_t) buffer[0] >> 3;
			  // Write the string to buffer for transmission
			  sprintf((char*)buffer, "CHIP I.D. Register value is: %x\r\n",val);
		  }

	  }
	  // Send string over serial.
	  CDC_Transmit_FS(buffer, sizeof(buffer));
	  // Repeat every 500ms
	  HAL_Delay(500);
  }
}
```

Rezultatas, kaip ir tikėtasi, yra 1c (šešioliktainėje sistemoje).
<p align="center">
<img src="https://user-images.githubusercontent.com/47836357/148701677-14fe2643-f422-4da1-ba03-153bbc75968a.png">
</p>
# 5 Užduotis
***Susipažinę su judesio jutiklio aprašymu, gaukite vienos ašies pagreitį***
STM savo plokštėms duoda Board Support Package. Tai yra driverių paketas, kuris leidžia vartotojams lengvai kontroliuoti išorinius įrenginius. "The BSP (board support package) drivers are part of the STM32Cube MCU and MPU Packages based on the HAL drivers, and provide a set of high-level APIs relative to the hardware components and features [...]. The BSP drivers allow quick access to the board services using high-level APIs, without any specific configuration as the link with the HAL and the external components is made intrinsically within the driver." Kad neišradinėti dviračio ir nebandyti perrašinėti kodo, šioje užduotyje naudojau BSP suteikiamas funkcijas. Jas į projektą galima įsikelti iš STM32CubeIDE automatiškai atsiūsto paketo, tačiau reikalingus failus įkeliau į patį projektą, kad nieko nereiktų papildomai daryti.

Pirma paleidžiama BSP_ACCELERO_Init() funkcija, kuri inicijuoja akselerometrą ir sukonfiguruoja jo parametrus. Tada naudodami BSP_ACCELERO_GetXYZ() funkciją gauname 3 ašių akseleracijos vertes. Pasiemame z-ašies vertę (nes plokštę padėjus ant stalo z-ašies vertė bus apie 1 g) ir suinčiame ją per serial port'ą.
```c
  uint8_t buffer[40];
  int16_t xyz[3] = {0};
  int16_t z_val;
  while (1)
  {
	// Initialise the accelerometer
	  if(BSP_ACCELERO_Init() != HAL_OK)
	   {
		  strcpy((char*) buffer, "Accelerometer initialisation Error\r\n");
	   }
	  // Get XYZ values
	  BSP_ACCELERO_GetXYZ(xyz);
	  // Get z-value
	  z_val = xyz[2];

	  sprintf((char*)buffer, "Z-axis acceleration is: %d mG\r\n", z_val);

	  // Print z value
	  CDC_Transmit_FS(buffer, sizeof(buffer));
	  // Repeat every 500ms
	  HAL_Delay(500);
}
}
```
Video demonstracija:
https://user-images.githubusercontent.com/47836357/148703935-c91a466c-57b7-4971-82b8-bcd438a440f5.mp4


