; Test suite for SRFI-95
;
; $Id$

(cond-expand (srfi-95))

(define (writeln . xs)
  (for-each display xs)
  (newline))

(define (fail token . more)
  (writeln "Error: test failed: " token)
  #f)

(or (equal? (sorted? '("a" "bcd" "cab" "cb") string<?)
            #t)
    (fail 'sorted?:list))

(or (equal? (sorted? '#("a" "bcd" "cab" "cb") string<?)
            #t)
    (fail 'sorted?:vector))

(or (equal? (sorted? "abcdde" char<?)
            #t)
    (fail 'sorted?:string))

(or (equal? (sorted? (make-array (A:fixN32b 4) 6) <)
            #t)
    (fail 'sorted?:array))

(or (equal? (sorted? '("a" "bcd" "ccab" "cb") string<?)
            #f)
    (fail 'sorted?:list))

(or (equal? (sorted? '#("a" "bcd" "ccab" "cb") string<?)
            #f)
    (fail 'sorted?:vector))

(or (equal? (sorted? "abedde" char<?)
            #f)
    (fail 'sorted?:string))

(or (equal? (sorted? '("xa" "bcd" "cab" "cb") string<? (lambda (x) "aaa"))
            #t)
    (fail 'sorted?:list:key))

(or (equal? (sorted? '#("xa" "bcd" "cab" "cb") string<? (lambda (x) "aaa"))
            #t)
    (fail 'sorted?:vector:key))

(or (equal? (sorted? "xabcdde" char<? (lambda (x) #\a))
            #t)
    (fail 'sorted?:string:key))

(or (equal? (sorted? (list->array 1 (A:fixN32b 4) '(1 2 -3 4 5))
                     <
                     (lambda (x) x))
            #f)
    (fail 'sorted?:array:key1))

(or (equal? (sorted? (list->array 1 (A:fixN32b 4) '(1 2 -3 4 5))
                     <
                     (lambda (x) (* x x)))
            #t)
    (fail 'sorted?:array:key2))

(or (equal? (merge '() '() <)
            '())
    (fail 'merge:empty2))

(or (equal? (merge '(6) '() <)
            '(6))
    (fail 'merge:empty1a))

(or (equal? (merge '() '(7) <)
            '(7))
    (fail 'merge:empty1b))

(or (equal? (merge '(1 4 9 16) '(2 4 8 16 32) <)
            '(1 2 4 4 8 9 16 16 32))
    (fail 'merge:list))

(or (equal? (merge '() '() < (lambda (x) (* x x)))
            '())
    (fail 'merge:empty2-key))

(or (equal? (merge '(6) '() < (lambda (x) (* x x)))
            '(6))
    (fail 'merge:empty1a-key))

(or (equal? (merge '() '(7) < (lambda (x) (* x x)))
            '(7))
    (fail 'merge:empty1b-key))

(or (equal? (merge '(1 4 9 16) '(2 4 -8 16 32) < (lambda (x) (* x x)))
            '(1 2 4 4 -8 9 16 16 32))
    (fail 'merge:list-key))

(or (equal? (merge! '() '() <)
            '())
    (fail 'merge!:empty2))

(or (equal? (merge! (list 6) '() <)
            '(6))
    (fail 'merge!:empty1a))

(or (equal? (merge! '() (list 7) <)
            '(7))
    (fail 'merge!:empty1b))

(or (equal? (merge! (list 1 4 9 16) (list 2 4 8 16 32) <)
            '(1 2 4 4 8 9 16 16 32))
    (fail 'merge!:list))

(or (equal? (merge! '() '() < (lambda (x) (* x x)))
            '())
    (fail 'merge!:empty2-key))

(or (equal? (merge! (list 6) '() < (lambda (x) (* x x)))
            '(6))
    (fail 'merge!:empty1a-key))

(or (equal? (merge! '() (list 7) < (lambda (x) (* x x)))
            '(7))
    (fail 'merge!:empty1b-key))

(or (equal? (merge! (list 1 4 9 16) (list 2 4 -8 16 32) < (lambda (x) (* x x)))
            '(1 2 4 4 -8 9 16 16 32))
    (fail 'merge!:list-key))

(or (equal? (sort '() >)
            '())
    (fail 'sort:empty))

(or (equal? (sort '(1 4 9 16 25 20 15 10 5) >)
            '(25 20 16 15 10 9 5 4 1))
    (fail 'sort:list))

(or (equal? (sort '#() >)
            '#())
    (fail 'sort:emptyvec))

(or (equal? (sort '#(1 4 9 16 25 20 15 10 5) >)
            '#(25 20 16 15 10 9 5 4 1))
    (fail 'sort:vector))

(or (equal? (sort "" char>?)
            "")
    (fail 'sort:emptystring))

(or (equal? (sort "aeioubcdf" char>?)
            "uoifedcba")
    (fail 'sort:string))

(or (equal? (sort '() > (lambda (x) (* x x)))
            '())
    (fail 'sort:empty-key))

(or (equal? (sort '(1 4 9 16 25 -20 15 -10 5) > (lambda (x) (* x x)))
            '(25 -20 16 15 -10 9 5 4 1))
    (fail 'sort:list-key))

(or (equal? (sort '#() > (lambda (x) (* x x)))
            '#())
    (fail 'sort:emptyvec-key))

(or (equal? (sort '#(1 4 9 -16 25 20 15 10 -5) > (lambda (x) (* x x)))
            '#(25 20 -16 15 10 9 -5 4 1))
    (fail 'sort:vector-key))

(or (equal? (sort "" char>? char-upcase)
            "")
    (fail 'sort:emptystring-key))

(or (equal? (sort "aeioubcdf" char>? char-downcase)
            "uoifedcba")
    (fail 'sort:string-key))

(or (equal? (sort! '() >)
            '())
    (fail 'sort!:empty))

(or (equal? (sort! (list 1 4 9 16 25 20 15 10 5) >)
            '(25 20 16 15 10 9 5 4 1))
    (fail 'sort!:list))

(or (equal? (sort! (vector) >)
            '#())
    (fail 'sort!:emptyvec))

(or (equal? (sort! (vector 1 4 9 16 25 20 15 10 5) >)
            '#(25 20 16 15 10 9 5 4 1))
    (fail 'sort!:vector))

(or (equal? (sort! "" char>?)
            "")
    (fail 'sort!:emptystring))

(or (equal? (sort! (string #\a #\e #\i #\o #\u #\b #\c #\d #\f) char>?)
            "uoifedcba")
    (fail 'sort!:string))

(or (equal? (sort! '() > (lambda (x) (* x x)))
            '())
    (fail 'sort!:empty-key))

(or (equal? (sort! (list 1 4 9 16 25 -20 15 -10 5) > (lambda (x) (* x x)))
            '(25 -20 16 15 -10 9 5 4 1))
    (fail 'sort!:list-key))

(or (equal? (sort! '#() > (lambda (x) (* x x)))
            '#())
    (fail 'sort!:emptyvec-key))

(or (let* ((v (vector 1 4 9 -16 25 20 15 10 -5))
           (w (sort! v > (lambda (x) (* x x)))))
      (and (equal? w
                  '#(25 20 -16 15 10 9 -5 4 1))
           (eq? v w)))
    (fail 'sort!:vector-key))

(or (let* ((s "")
           (s2 (sort! s char>? char-downcase)))
      (and (equal? s2
                   "")
           (eq? s s2)))
    (fail 'sort!:emptystring-key))

(or (equal? (sort! (string #\a #\e #\i #\o #\u #\b #\c #\d #\f)
                   char>?
                   char-upcase)
            "uoifedcba")
    (fail 'sort!:string-key))

(writeln "Done.")
