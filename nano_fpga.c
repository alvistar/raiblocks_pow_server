// Amazon FPGA Hardware Development Kit
//
// Copyright 2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Amazon Software License (the "License"). You may not use
// this file except in compliance with the License. A copy of the License is
// located at
//
//    http://aws.amazon.com/asl/
//
// or in the "license" file accompanying this file. This file is distributed on
// an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, express or
// implied. See the License for the specific language governing permissions and
// limitations under the License.
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#include <fpga_pci.h>
#include <fpga_mgmt.h>
#include <utils/lcd.h>

#include <utils/sh_dpi_tasks.h>

/* use the stdout logger for printing debug information  */

const struct logger *logger = &logger_stdout;
/*
 * pci_vendor_id and pci_device_id values below are Amazon's and avaliable to use for a given FPGA slot. 
 * Users may replace these with their own if allocated to them by PCI SIG
 */
static uint16_t pci_vendor_id = 0x1D0F; /* Amazon PCI Vendor ID */
static uint16_t pci_device_id = 0xF000; /* PCI Device ID preassigned by Amazon for F1 applications */

/*
 * check if the corresponding AFI for hello_world is loaded
 */
int check_afi_ready(int slot_id);

int pow_( uint8_t *pin, uint8_t *pout );

uint32_t byte_swap(uint32_t value);
 
uint32_t byte_swap(uint32_t value) {
    uint32_t swapped_value = 0;
    int b;
    for (b = 0; b < 4; b++) {
        swapped_value |= ((value >> (b * 8)) & 0xff) << (8 * (3-b));
    }
    return swapped_value;
}

#define cv32( a )  ( (uint32_t)(*(a)) + (((uint32_t)(*(a+1)))<<8 ) + (((uint32_t)(*(a+2)))<<16 ) + (((uint32_t)(*(a+3)))<<24 )) 

int check_afi_ready(int slot_id) {
   struct fpga_mgmt_image_info info = {0};
   int rc;

   /* get local image description, contains status, vendor id, and device id. */
   rc = fpga_mgmt_describe_local_image(slot_id, &info,0);
   fail_on(rc, out, "Unable to get AFI information from slot %d. Are you running as root?",slot_id);

   /* check to see if the slot is ready */
   if (info.status != FPGA_STATUS_LOADED) {
     rc = 1;
     fail_on(rc, out, "AFI in Slot %d is not in READY state !", slot_id);
   }

   printf("AFI PCI  Vendor ID: 0x%x, Device ID 0x%x\n",
          info.spec.map[FPGA_APP_PF].vendor_id,
          info.spec.map[FPGA_APP_PF].device_id);

   /* confirm that the AFI that we expect is in fact loaded */
   if (info.spec.map[FPGA_APP_PF].vendor_id != pci_vendor_id ||
       info.spec.map[FPGA_APP_PF].device_id != pci_device_id) {
     printf("AFI does not show expected PCI vendor id and device ID. If the AFI "
            "was just loaded, it might need a rescan. Rescanning now.\n");

     rc = fpga_pci_rescan_slot_app_pfs(slot_id);
     fail_on(rc, out, "Unable to update PF for slot %d",slot_id);
     /* get local image description, contains status, vendor id, and device id. */
     rc = fpga_mgmt_describe_local_image(slot_id, &info,0);
     fail_on(rc, out, "Unable to get AFI information from slot %d",slot_id);

     printf("AFI PCI  Vendor ID: 0x%x, Device ID 0x%x\n",
            info.spec.map[FPGA_APP_PF].vendor_id,
            info.spec.map[FPGA_APP_PF].device_id);

     /* confirm that the AFI that we expect is in fact loaded after rescan */
     if (info.spec.map[FPGA_APP_PF].vendor_id != pci_vendor_id ||
         info.spec.map[FPGA_APP_PF].device_id != pci_device_id) {
       rc = 1;
       fail_on(rc, out, "The PCI vendor id and device of the loaded AFI are not "
               "the expected values.");
     }
   }

   return rc;
 out:
   return 1;
 }

int init() {
    int slot_id = 0;
    int rc;

    printf("===== Initializing FPGA =====\n");
    /* initialize the fpga_pci library so we could have access to FPGA PCIe from this applications */
    rc = fpga_pci_init();
    fail_on(rc, out, "Unable to initialize the fpga_pci library");

    rc = check_afi_ready(slot_id);
    fail_on(rc, out, "AFI not ready");

    return rc;
    
    out:
        return 1;
}

int pow_( uint8_t *pin, uint8_t *pout )
{
    int slot_id = 0;
    int rc;
    uint32_t value;
    
    printf("pow START >>>>>>>>>>");

    int i;

    for (i = 0; i < 32; i++)
        printf("%02X", pin[i]);

    printf("\n");
    /* Accessing the CL registers via AppPF BAR0, which maps to sh_cl_ocl_ AXI-Lite bus between AWS FPGA Shell and the CL*/
    // rc = peek_poke_example(value, slot_id, FPGA_APP_PF, APP_PF_BAR0);
 
    /* pci_bar_handle_t is a handler for an address space exposed by one PCI BAR on one of the PCI PFs of the FPGA */
    pci_bar_handle_t pci_bar_handle = PCI_BAR_HANDLE_INIT;
    
    /* attach to the fpga, with a pci_bar_handle out param
     * To attach to multiple slots or BARs, call this function multiple times,
     * saving the pci_bar_handle to specify which address space to interact with in
     * other API calls.
     * This function accepts the slot_id, physical function, and bar number
     */

    rc = fpga_pci_attach(slot_id, FPGA_APP_PF, APP_PF_BAR0, 0, &pci_bar_handle);
    fail_on(rc, out, "Unable to attach to the AFI on slot id %d", slot_id);
 
    fpga_pci_poke( pci_bar_handle, 0x00000524, cv32(pin+28) ); //0x1237f90c );
    fpga_pci_poke( pci_bar_handle, 0x00000520, cv32(pin+24 ) ); //0x9bc52c8 );
    fpga_pci_poke( pci_bar_handle, 0x0000051c, cv32(pin+20) ); //0x43ca3735 );
    fpga_pci_poke( pci_bar_handle, 0x00000518, cv32(pin+16) ); //0x280b91a9 );
    fpga_pci_poke( pci_bar_handle, 0x00000514, cv32(pin+12) ); //0x6ac5ab76 );
    fpga_pci_poke( pci_bar_handle, 0x00000510, cv32(pin+8) ); //0x7265255b );
    fpga_pci_poke( pci_bar_handle, 0x0000050c, cv32(pin+4) ); //0x44028777 );
    fpga_pci_poke( pci_bar_handle, 0x00000508, cv32(pin ) ); //0x75b8e5c8 );

    /* read it back and print it out; you should expect the byte order to be
     * reversed (That's what this CL does) */
    do    {
     rc = fpga_pci_peek(pci_bar_handle, 0x00000510, &value);
     fail_on(rc, out, "Unable to read read from the fpga !");
    }while( value == 0 );
    fpga_pci_peek(pci_bar_handle, 0x00000504, &value);
    *pout = ( value >> 24 )& 0xFF;
    *(pout+1) = ( value >> 16 )& 0xFF;
    *(pout+2) = ( value >> 8 )& 0xFF;
    *(pout+3) = ( value >> 0 )& 0xFF;
    fpga_pci_peek(pci_bar_handle, 0x00000500, &value);
    *(pout+4) = ( value >> 24 )& 0xFF;
    *(pout+5) = ( value >> 16 )& 0xFF;
    *(pout+6) = ( value >> 8 )& 0xFF;
    *(pout+7) = ( value >> 0 )& 0xFF;
    fpga_pci_peek(pci_bar_handle, 0x0000050C, &value);
    *(pout+8) = ( value >> 24 )& 0xFF;
    *(pout+9) = ( value >> 16 )& 0xFF;
    *(pout+10) = ( value >> 8 )& 0xFF;
    *(pout+11) = ( value >> 0 )& 0xFF;
    fpga_pci_peek(pci_bar_handle, 0x00000508, &value);
    *(pout+12) = ( value >> 24 )& 0xFF;
    *(pout+13) = ( value >> 16 )& 0xFF;
    *(pout+14) = ( value >> 8 )& 0xFF;
    *(pout+15) = ( value >> 0 )& 0xFF;
    fpga_pci_peek(pci_bar_handle, 0x00000510, &value);
    *(pout+16) = ( value >> 24 )& 0xFF;
    *(pout+17) = ( value >> 16 )& 0xFF;
    *(pout+18) = ( value >> 8 )& 0xFF;
    *(pout+19) = ( value >> 0 )& 0xFF;
out:
    if (pci_bar_handle >= 0) {
        rc = fpga_pci_detach(pci_bar_handle);
        if (rc) {
            printf("Failure while detaching from the fpga.\n");
        }
    }
    return rc;
}
