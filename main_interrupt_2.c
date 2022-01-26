#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xgpio.h"
#include "xiic.h"
#include "Xscugic.h"
#include "xparameters.h"
#include "xparameters_ps.h"


//----------------------------------------------------
// PROTOTYPE FUNCTIONS
//----------------------------------------------------

static int InterruptSystemSetup(XScuGic *XScuGicInstancePtr);
static void Interrupt_SWS_BTN_Handler(void);


//----------------------------------------------------



#define LEDS_ID					XPAR_LEDS_PORT_DEVICE_ID
#define BT_SW_ID				XPAR_SWS_BTNS_PORT_DEVICE_ID

#define IIC_ID					XPAR_IIC_PORT_DEVICE_ID
#define IIC_Baseaddress			XPAR_IIC_PORT_BASEADDR

#define GIC_ID					XPAR_PS7_SCUGIC_0_DEVICE_ID

#define REG_INTR_SW_BTN			XPAR_FABRIC_SWS_BTNS_PORT_IP2INTC_IRPT_INTR
#define REG_INTR_IIC			XPAR_FABRIC_IIC_PORT_IIC2INTC_IRPT_INTR

#define SWS_INT					XGPIO_IR_CH1_MASK
#define BTN_INT					XGPIO_IR_CH2_MASK
#define SWS_BTN_INT				XGPIO_IR_MASK





XGpio MesBTN_SW_driver;
XGpio MesLEDs_driver;

XIic MonIIC_driver;


XScuGic MonGIC_driver;
XScuGic_Config* MonGIC_driver_config;

u8 *data8;
int status=0;


int main()
{
    //u8 priority_btn=0x70;
    //u8 trigger_btn=0xDC;
	init_platform();
    //u32 leds;

    // initialisation périphériques
    XIic_Initialize(&MonIIC_driver, IIC_ID); // la configuration est créée dans cette initialisation


	XGpio_Initialize(&MesBTN_SW_driver,BT_SW_ID);
	XGpio_Initialize(&MesLEDs_driver,LEDS_ID);

	// initialisation GIC
	MonGIC_driver_config= XScuGic_LookupConfig(GIC_ID);
	XScuGic_CfgInitialize(&MonGIC_driver,MonGIC_driver_config,MonGIC_driver_config -> CpuBaseAddress);
	InterruptSystemSetup(&MonGIC_driver);


	//activation des interruptions GPIO
	XGpio_InterruptGlobalEnable(&MesBTN_SW_driver);
	XGpio_InterruptEnable(&MesBTN_SW_driver, SWS_BTN_INT);



	// connexion entre interruption et gestion d'interruption GPIO
	XScuGic_Connect(&MonGIC_driver,REG_INTR_SW_BTN,(Xil_ExceptionHandler)Interrupt_SWS_BTN_Handler,&MesBTN_SW_driver.BaseAddress);
	XScuGic_Enable(&MonGIC_driver,REG_INTR_SW_BTN);


	// connexion entre interruption et gestion d'interruption IIC
	XScuGic_Connect(&MonGIC_driver,REG_INTR_IIC,(Xil_ExceptionHandler)XIic_InterruptHandler,&MonIIC_driver);
	XScuGic_Enable(&MonGIC_driver,REG_INTR_IIC);
	XIic_IntrGlobalEnable(IIC_Baseaddress);


	//démarrage de mon bus IIC
	XIic_Start(&MonIIC_driver); // doit être ecrit apres la connection de l'ISR

    XGpio_SetDataDirection(&MesBTN_SW_driver, 1,0xFFFFFFFF);
    XGpio_SetDataDirection(&MesBTN_SW_driver, 2,0xFFFFFFFF);
    XGpio_SetDataDirection(&MesLEDs_driver, 1,0xFFFFFFF0);



    while(1)
    {

    }

    print("Hello World\n\r");
    print("Successfully ran Hello World application");
    cleanup_platform();
    return 0;
}


int InterruptSystemSetup(XScuGic *XScuGicInstancePtr)
{
	// Register GIC interrupt handler

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
			 	 	 	 	 	 (Xil_ExceptionHandler)XScuGic_InterruptHandler,
			 	 	 	 	 	 XScuGicInstancePtr);
	Xil_ExceptionEnable();


	return XST_SUCCESS;

}

void Interrupt_SWS_BTN_Handler(void)
{
	u32 LEDs;
	//désactivation des interruptions provenant des GPIOs
	XGpio_InterruptGlobalDisable(&MesBTN_SW_driver);

	LEDs=( XGpio_DiscreteRead(&MesBTN_SW_driver,1) )^( XGpio_DiscreteRead(&MesBTN_SW_driver,2) );
	if (LEDs==0x01)
		{
		XIic_SetAddress(&MonIIC_driver, XII_ADDR_TO_SEND_TYPE, 0x64);
		status=XIic_MasterRecv(&MonIIC_driver, data8, 1);
	    }


	XGpio_DiscreteWrite(&MesLEDs_driver,1,LEDs);

	// remise à zéro et activation des interruptions provenant des GPIOs
	XGpio_InterruptClear(&MesBTN_SW_driver, SWS_BTN_INT);
	XGpio_InterruptGlobalEnable(&MesBTN_SW_driver);
}


