import fpga_interface
from pyblake2 import blake2b

def pow_threshold(check):
    if check > b'\xFF\xFF\xFF\xC0\x00\x00\x00\x00':
        return True
    else:
        return False

def pow_validate(pow, hash):
    pow_data = bytearray.fromhex(pow)
    hash_data = bytearray.fromhex(hash)

    h = blake2b(digest_size=8)
    pow_data.reverse()
    h.update(pow_data)
    h.update(hash_data)
    final = bytearray(h.digest())
    final.reverse()

    return pow_threshold(final)

print(fpga_interface.init_fpga())

result = fpga_interface.do_pow("C8E5B875778702445B25657276ABC56AA9910B283537CA438B2CC59B0CF93712")

ba = bytearray(result)

print("FPGA Output: {}".format(ba.hex()))

pow = ba[8:16]

print("Pow:{}".format(pow.hex()))

print(pow_validate(pow.hex(),"C8E5B875778702445B25657276ABC56AA9910B283537CA438B2CC59B0CF93712"))