; DO NOT EDIT THIS FILE. Edit the config file and rerun "config".

(define $gc.ephemeral 0)
(define $gc.tenuring 1)
(define $gc.full 2)
(define $mstat.wallocated-hi 0)
(define $mstat.wallocated-lo 1)
(define $mstat.wcollected-hi 2)
(define $mstat.wcollected-lo 3)
(define $mstat.wcopied-hi 4)
(define $mstat.wcopied-lo 5)
(define $mstat.gctime 6)
(define $mstat.wlive 7)
(define $mstat.gc-last-gen 8)
(define $mstat.gc-last-type 9)
(define $mstat.generations 10)
(define $mstat.g-gc-count 0)
(define $mstat.g-prom-count 1)
(define $mstat.g-gctime 2)
(define $mstat.g.wlive 3)
(define $mstat.remsets 11)
(define $mstat.r-apool 0)
(define $mstat.r-upool 1)
(define $mstat.r-ahash 2)
(define $mstat.r-uhash 3)
(define $mstat.r-hrec-hi 4)
(define $mstat.r-hrec-lo 5)
(define $mstat.r-hrem-hi 6)
(define $mstat.r-hrem-lo 7)
(define $mstat.r-hscan-hi 8)
(define $mstat.r-hscan-lo 9)
(define $mstat.r-wscan-hi 10)
(define $mstat.r-wscan-lo 11)
(define $mstat.r-ssbrec-hi 12)
(define $mstat.r-ssbrec-lo 13)
(define $mstat.fflushed-hi 12)
(define $mstat.fflushed-lo 13)
(define $mstat.wflushed-hi 14)
(define $mstat.wflushed-lo 15)
(define $mstat.stk-created 16)
(define $mstat.frestored-hi 17)
(define $mstat.frestored-lo 18)
(define $mstat.words-heap 19)
(define $mstat.words-remset 20)
(define $mstat.words-rts 21)
(define $mstat.swb-assign 22)
(define $mstat.swb-lhs-ok 23)
(define $mstat.swb-rhs-const 24)
(define $mstat.swb-not-xgen 25)
(define $mstat.swb-trans 26)
(define $mstat.rtime 27)
(define $mstat.stime 28)
(define $mstat.utime 29)
(define $mstat.minfaults 30)
(define $mstat.majfaults 31)
(define $mstat.vsize 32)
(define $g.reg0 12)
(define $r.reg8 44)
(define $r.reg9 48)
(define $r.reg10 52)
(define $r.reg11 56)
(define $r.reg12 60)
(define $r.reg13 64)
(define $r.reg14 68)
(define $r.reg15 72)
(define $r.reg16 76)
(define $r.reg17 80)
(define $r.reg18 84)
(define $r.reg19 88)
(define $r.reg20 92)
(define $r.reg21 96)
(define $r.reg22 100)
(define $r.reg23 104)
(define $r.reg24 108)
(define $r.reg25 112)
(define $r.reg26 116)
(define $r.reg27 120)
(define $r.reg28 124)
(define $r.reg29 128)
(define $r.reg30 132)
(define $r.reg31 136)
(define $g.stkbot 180)
(define $m.alloc 1024)
(define $m.alloci 1032)
(define $m.gc 1040)
(define $m.addtrans 1048)
(define $m.stkoflow 1056)
(define $m.stkuflow 1072)
(define $m.creg 1080)
(define $m.creg-set! 1088)
(define $m.add 1096)
(define $m.subtract 1104)
(define $m.multiply 1112)
(define $m.quotient 1120)
(define $m.remainder 1128)
(define $m.divide 1136)
(define $m.modulo 1144)
(define $m.negate 1152)
(define $m.numeq 1160)
(define $m.numlt 1168)
(define $m.numle 1176)
(define $m.numgt 1184)
(define $m.numge 1192)
(define $m.zerop 1200)
(define $m.complexp 1208)
(define $m.realp 1216)
(define $m.rationalp 1224)
(define $m.integerp 1232)
(define $m.exactp 1240)
(define $m.inexactp 1248)
(define $m.exact->inexact 1256)
(define $m.inexact->exact 1264)
(define $m.make-rectangular 1272)
(define $m.real-part 1280)
(define $m.imag-part 1288)
(define $m.sqrt 1296)
(define $m.round 1304)
(define $m.truncate 1312)
(define $m.apply 1320)
(define $m.varargs 1328)
(define $m.typetag 1336)
(define $m.typetag-set 1344)
(define $m.break 1352)
(define $m.eqv 1360)
(define $m.partial-list->vector 1368)
(define $m.timer-exception 1376)
(define $m.exception 1384)
(define $m.singlestep 1392)
(define $m.syscall 1400)
(define $m.bvlcmp 1408)
(define $m.enable-interrupts 1416)
(define $m.disable-interrupts 1424)
