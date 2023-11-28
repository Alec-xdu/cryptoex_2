from math import gcd

p, q, result, e = 1009, 3643, 0, 3
phi = (p - 1) * (q - 1)

for e in range(1, phi):
	if gcd(e, phi) == 1 and gcd(e - 1, q - 1) == 2 and gcd(e - 1, p - 1) == 2:
		result += e

print("sum of es =", result)
