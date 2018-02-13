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

#ifdef SV_TEST
   #include "fpga_pci_sv.h"
#else
   #include <fpga_pci.h>
   #include <fpga_mgmt.h>
   #include <utils/lcd.h>
#endif

#include <utils/sh_dpi_tasks.h>

/* Constants determined by the CL */
/* a set of register offsets; this CL has only one */
/* these register addresses should match the addresses in */
/* /aws-fpga/hdk/cl/examples/common/cl_common_defines.vh */
/* SV_TEST macro should be set if SW/HW co-simulation should be enabled */

#define HELLO_WORLD_REG_ADDR UINT64_C(0x500)
#define VLED_REG_ADDR	UINT64_C(0x504)

/* use the stdout logger for printing debug information  */
#ifndef SV_TEST
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
/*
 * An example to attach to an arbitrary slot, pf, and bar with register access.
 */
int peek_poke_example(uint32_t value, int slot_id, int pf_id, int bar_id);

int pow_( uint8_t *pin, uint8_t *pout );

void usage(char* program_name) {
    printf("usage: %s [--slot <slot-id>][<poke-value>]\n", program_name);
}

uint32_t byte_swap(uint32_t value);
 
#endif

uint32_t byte_swap(uint32_t value) {
    uint32_t swapped_value = 0;
    int b;
    for (b = 0; b < 4; b++) {
        swapped_value |= ((value >> (b * 8)) & 0xff) << (8 * (3-b));
    }
    return swapped_value;
}

#define cv32( a )  ( (uint32_t)(*(a)) + (((uint32_t)(*(a+1)))<<8 ) + (((uint32_t)(*(a+2)))<<16 ) + (((uint32_t)(*(a+3)))<<24 )) 

int init() {
    int slot_id = 0;
    int rc;
    uint32_t value = 0xefbeadde;

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

#ifdef SV_TEST
void test_main(uint32_t *exit_code) {
#else
int main(int argc, char **argv) {
#endif
    //The statements within SCOPE ifdef below are needed for HW/SW co-simulation with VCS
    #ifdef SCOPE
      svScope scope;
      scope = svGetScopeFromName("tb");
      svSetScope(scope);
    #endif

    uint32_t value = 0xefbeadde;
    int slot_id = 0;
    int rc;
    uint8_t pin[] = {0xC8, 0xE5, 0xB8, 0x75, 0x77, 0x87, 0x02, 0x44, 0x5B, 0x25, 0x65, 0x72,
    	              0x76, 0xAB, 0xC5, 0x6A, 0xA9, 0x91, 0x0B, 0x28, 0x35, 0x37, 0xCA, 0x43,
    	              0x8B, 0x2C, 0xC5, 0x9B, 0x0C, 0xF9, 0x37, 0x12};
    uint8_t pout[20];
    
#ifndef SV_TEST
    // Process command line args
    {
        int i;
        int value_set = 0;
        for (i = 1; i < argc; i++) {
            if (!strcmp(argv[i], "--slot")) {
                i++;
                if (i >= argc) {
                    printf("error: missing slot-id\n");
                    usage(argv[0]);
                    return 1;
                }
                sscanf(argv[i], "%d", &slot_id);
            } else if (!value_set) {
                sscanf(argv[i], "%x", &value);
                value_set = 1;
            } else {
                printf("error: Invalid arg: %s", argv[i]);
                usage(argv[0]);
                return 1;
            }
        }
    }
#endif

    /* initialize the fpga_pci library so we could have access to FPGA PCIe from this applications */
    rc = fpga_pci_init();
    fail_on(rc, out, "Unable to initialize the fpga_pci library");

#ifndef SV_TEST
    rc = check_afi_ready(slot_id);
#endif

    fail_on(rc, out, "AFI not ready");
    
    /* Accessing the CL registers via AppPF BAR0, which maps to sh_cl_ocl_ AXI-Lite bus between AWS FPGA Shell and the CL*/

    printf("===== Starting with peek_poke_example =====\n");
    rc = peek_poke_example(value, slot_id, FPGA_APP_PF, APP_PF_BAR0);
    fail_on(rc, out, "peek-poke example failed");
    pow_( pin, pout );
    printf( "0x%08X0x%08X\n", cv32(pout),cv32(pout+4) );
    printf( "0x%08X0x%08X\n", cv32(pout+8),cv32(pout+12) );
    printf( "0x%08X\n", cv32(pout+20) );
    printf("Developers are encouraged to modify the Virtual DIP Switch by calling the linux shell command to demonstrate how AWS FPGA Virtual DIP switches can be used to change a CustomLogic functionality:\n");
    printf("$ fpga-set-virtual-dip-switch -S (slot-id) -D (16 digit setting)\n\n");
    printf("In this example, setting a virtual DIP switch to zero clears the corresponding LED, even if the peek-poke example would set it to 1.\nFor instance:\n");

    printf(
        "# sudo fpga-set-virtual-dip-switch -S 0 -D 1111111111111111\n"
        "# sudo fpga-get-virtual-led  -S 0\n"
        "FPGA slot id 0 have the following Virtual LED:\n"
        "1010-1101-1101-1110\n"
        "# sudo fpga-set-virtual-dip-switch -S 0 -D 0000000000000000\n"
        "# sudo fpga-get-virtual-led  -S 0\n"
        "FPGA slot id 0 have the following Virtual LED:\n"
        "0000-0000-0000-0000\n"
    );

#ifndef SV_TEST
    return rc;
    
out:
    return 1;
#else

out:
   *exit_code = 0;
#endif
}

/* As HW simulation test is not run on a AFI, the below function is not valid */
#ifndef SV_TEST

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

#endif

/*
 * An example to attach to an arbitrary slot, pf, and bar with register access.
 */
int peek_poke_example(uint32_t value, int slot_id, int pf_id, int bar_id) {
    int rc;
    uint32_t value1;

    /* pci_bar_handle_t is a handler for an address space exposed by one PCI BAR on one of the PCI PFs of the FPGA */

    pci_bar_handle_t pci_bar_handle = PCI_BAR_HANDLE_INIT;

    
    /* attach to the fpga, with a pci_bar_handle out param
     * To attach to multiple slots or BARs, call this function multiple times,
     * saving the pci_bar_handle to specify which address space to interact with in
     * other API calls.
     * This function accepts the slot_id, physical function, and bar number
     */
#ifndef SV_TEST
    rc = fpga_pci_attach(slot_id, pf_id, bar_id, 0, &pci_bar_handle);
#endif
    fail_on(rc, out, "Unable to attach to the AFI on slot id %d", slot_id);
    
    /* write a value into the mapped address space */
    uint32_t expected = byte_swap(value);
    printf("Writing 0x%08x to HELLO_WORLD register (0x%016lx)\n", value, HELLO_WORLD_REG_ADDR);
    rc = fpga_pci_poke(pci_bar_handle, HELLO_WORLD_REG_ADDR, value);

    fail_on(rc, out, "Unable to write to the fpga !");
    rc = fpga_pci_peek(pci_bar_handle, 0x00000514, &value);
     printf("register: 0x%x\n", value);

    fpga_pci_poke( pci_bar_handle, 0x00000520, 0x9bc52c8b );
    fpga_pci_poke( pci_bar_handle, 0x0000051c, 0x43ca3735 );
    fpga_pci_poke( pci_bar_handle, 0x00000518, 0x280b91a9 );
    fpga_pci_poke( pci_bar_handle, 0x00000514, 0x6ac5ab76 );
    fpga_pci_poke( pci_bar_handle, 0x00000510, 0x7265255b );
    fpga_pci_poke( pci_bar_handle, 0x0000050c, 0x44028777 );
    fpga_pci_poke( pci_bar_handle, 0x00000508, 0x75b8e5c8 );
//    fpga_pci_poke( pci_bar_handle, 0x00000504, 0xd2d0c5ad );
//    fpga_pci_poke( pci_bar_handle, 0x00000500, 0x9cc8ee73 );

    rc = fpga_pci_peek(pci_bar_handle, 0x00000514, &value);
     printf("register: 0x%x\n", value);

    fpga_pci_poke( pci_bar_handle, 0x00000524, 0x1237f90c );

    
    /* read it back and print it out; you should expect the byte order to be
     * reversed (That's what this CL does) */
    do    {
     rc = fpga_pci_peek(pci_bar_handle, 0x00000510, &value);
     fail_on(rc, out, "Unable to read read from the fpga !");
    }while( value == 0 );
    fpga_pci_peek(pci_bar_handle, 0x00000500, &value);
    fpga_pci_peek(pci_bar_handle, 0x00000504, &value1);
    printf("register: 0x%08x%08x\n", value1, value);
    fpga_pci_peek(pci_bar_handle, 0x00000508, &value);
    fpga_pci_peek(pci_bar_handle, 0x0000050c, &value1);
    printf("register: 0x%08x%08x\n", value1,value);
    fpga_pci_peek(pci_bar_handle, 0x00000510, &value);
    printf("tempo %d ms\n", ( value*8 )/1000 );

    if(value == expected) {
        printf("TEST PASSED");
        printf("Resulting value matched expected value 0x%x. It worked!\n", expected);
    }
    else{
        printf("TEST FAILED");
        printf("Resulting value did not match expected value 0x%x. Something didn't work.\n", expected);
    }
out:
    /* clean up */
    if (pci_bar_handle >= 0) {
        rc = fpga_pci_detach(pci_bar_handle);
        if (rc) {
            printf("Failure while detaching from the fpga.\n");
        }
    }

    /* if there is an error code, exit with status 1 */
    return (rc != 0 ? 1 : 0);
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
