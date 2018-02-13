
from libc.stdint cimport uint8_t
from cpython cimport array

cdef extern int pow_( uint8_t *pin, uint8_t *pout );
cdef extern int init();

def init_fpga():
  return init()

def do_pow(hash):
  hash_b = bytes.fromhex(hash)
  cdef array.array a = array.array('B', hash_b)
  cdef array.array int_array_template = array.array('B', [])
  cdef array.array b

  b = array.clone(int_array_template, 20, zero=True)

  #cdef uint8_t[:] pin = a
  #pow(pin, pin)
  pow_(a.data.as_uchars, b.data.as_uchars)
  return b