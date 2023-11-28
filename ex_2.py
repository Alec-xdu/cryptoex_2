from math import gcd
from Crypto.Util.number import getPrime


def invMod(m: int, n: int) -> int:
	s, t, d = extGcd(m, n)
	if gcd(m, n) != 1:
		exit(1)
	if d == 1:
		while s < 0:
			s += n
		return s


def extGcd(a, b):
	if b == 0:
		return 1, 0, a
	else:
		x,  y, egcd = extGcd(b, a % b)
		x, y = y, (x - (a // b) * y)
		return x, y, egcd


class RSAEncrypt:
	def __init__(self):
		self.primeSize = 512
		self.p = getPrime(self.primeSize)
		self.q = getPrime(self.primeSize)
		self.setNotEqualPnQ()
		self.n = self.p * self.q
		self.e = 3
		self.et = (self.p - 1) * (self.q - 1)
		self.d = invMod(self.e, self.et)
		self.publicKey = [self.e, self.n]
		self.privateKey = [self.d, self.n]

	def setNotEqualPnQ(self):
		while self.p == self.q:
			self.q = getPrime(self.primeSize)

	def encrypt(self, m: str) -> int:
		m = int(m.encode().hex(), 16)
		return pow(m, self.e, self.n)

	def decrypt(self, c: int) -> str:
		c = pow(c, self.d, self.n)
		return bytes.fromhex(hex(c)[2:]).decode('utf8')


if __name__ == '__main__':
	rsa = RSAEncrypt()
	ct = rsa.encrypt('test text')
	print(hex(ct))
	pt = rsa.decrypt(ct)
	print(pt)
