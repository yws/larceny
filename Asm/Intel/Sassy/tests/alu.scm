(
(adc al 100)
(add ax  1000)
(and eax 50000)
(cmp bl  100)
(or cx (word 1000))
(sbb edx (dword 50000))
(sub cx (byte 100))
(xor edx (byte 100))
(adc (dword (& eax (* edx 4) 100)) (dword 50000))
(add (dword (& edx)) (byte 100))
(and (word (& eax edx)) (word 1000))
(cmp (word (& eax edx)) (byte 100))
(or (byte (& eax (* edx 4) 100)) (byte 100))
(sbb bl bl)
(sub (& ebx) bl)
(xor cx  cx)
(adc (& eax edx) cx)
(add edx edx)
(and (& eax (* edx 4) 100) edx)
(cmp bl  (& ebx))
(or cx (& eax edx))
(sbb edx (& eax (* edx 4) 100))
)