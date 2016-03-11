;;; Linear-time (average case) algorithms for:
;;;
;;; Selecting the kth smallest element from an unsorted vector.
;;; Selecting the kth and (k+1)st smallest elements from an unsorted vector.
;;; Selecting the median from an unsorted vector.

;;; These procedures are part of SRFI 132 but are missing from
;;; its reference implementation as of 10 March 2016.

;;; Although the median can be found in linear time, SRFI 132
;;; is written as though the median must be found by sorting.
;;; Well, fie on that.

(define (vector-find-median < v knil . rest)
  (let* ((mean (if (null? rest)
                   (lambda (a b) (/ (+ a b) 2))
                   (car rest)))
         (n (vector-length v)))
    (cond ((zero? n)
           knil)
          ((odd? n)
           (%vector-select < v (quotient n 2) 0 n))
          (else
           (call-with-values
            (lambda () (%vector-select2 < v (- (quotient n 2) 1) 0 n))
            (lambda (a b)
              (mean a b)))))))

;;; For this procedure, however, the SRFI 132 specification
;;; demands the vector be sorted (by side effect).

(define (vector-find-median! < v knil . rest)
  (let* ((mean (if (null? rest)
                   (lambda (a b) (/ (+ a b) 2))
                   (car rest)))
         (n (vector-length v)))
    (vector-sort! < v)
    (cond ((zero? n)
           knil)
          ((odd? n)
           (vector-ref v (quotient n 2)))
          (else
           (mean (vector-ref v (- (quotient n 2) 1))
                 (vector-ref v (quotient n 2)))))))

;;; This could be made slightly more efficient, but who cares?

(define (vector-select! < v k . rest)
  (let* ((start (if (null? rest)
                    0
                    (car rest)))
         (end (if (and (pair? rest)
                       (pair? (cdr rest)))
                  (car (cdr rest))
                  (vector-length v))))
    (vector-sort! < v start end)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; For small ranges, sorting may be the fastest way to find the kth element.
;;; This threshold is not at all critical, and may not even be worthwhile.

(define just-sort-it-threshold 50)

;;; Given
;;;     an irreflexive total order <?
;;;     a vector v
;;;     an index k
;;;     an index start
;;;     an index end
;;; with
;;;     0 <= k < (- end start)
;;;     0 <= start < end <= (vector-length v)
;;; returns
;;;     (vector-ref (vector-sort <? (vector-copy v start end)) k)
;;; but is usually faster than that.

(define (%vector-select <? v k start end)
  (assert (and 'vector-select
               (procedure? <?)
               (vector? v)
               (exact-integer? k)
               (exact-integer? start)
               (exact-integer? end)
               (<= 0 k (- end start 1))
               (<= 0 start end (vector-length v))))
  (%%vector-select <? v k start end))

;;; Given
;;;     an irreflexive total order <?
;;;     a vector v
;;;     an index k
;;;     an index start
;;;     an index end
;;; with
;;;     0 <= k < (- end start 1)
;;;     0 <= start < end <= (vector-length v)
;;; returns two values:
;;;     (vector-ref (vector-sort <? (vector-copy v start end)) k)
;;;     (vector-ref (vector-sort <? (vector-copy v start end)) (+ k 1))
;;; but is usually faster than that.

(define (%vector-select2 <? v k start end)
  (assert (and 'vector-select
               (procedure? <?)
               (vector? v)
               (exact-integer? k)
               (exact-integer? start)
               (exact-integer? end)
               (<= 0 k (- end start 1 1))
               (<= 0 start end (vector-length v))))
  (%%vector-select2 <? v k start end))

;;; Like %vector-select, but its preconditions have been checked.

(define (%%vector-select <? v k start end)
  (let ((size (- end start)))
    (cond ((= 1 size)
           (vector-ref v (+ k start)))
          ((= 2 size)
           (cond ((<? (vector-ref v start)
                      (vector-ref v (+ start 1)))
                  (vector-ref v (+ k start)))
                 (else
                  (vector-ref v (+ (- 1 k) start)))))
          ((< size just-sort-it-threshold)
           (vector-ref (r6rs-vector-sort <? (r7rs-vector-copy v start end)) k))
          (else
           (let* ((ip (random-integer size))
                  (pivot (vector-ref v (+ start ip))))
             (call-with-values
              (lambda () (count-smaller <? pivot v start end 0 0))
              (lambda (count count2)
                (cond ((< k count)
                       (let* ((n count)
                              (v2 (make-vector n)))
                         (copy-smaller! <? pivot v2 0 v start end)
                         (%%vector-select <? v2 k 0 n)))
                      ((< k (+ count count2))
                       pivot)
                      (else
                       (let* ((n (- size count count2))
                              (v2 (make-vector n))
                              (k2 (- k count count2)))
                         (copy-bigger! <? pivot v2 0 v start end)
                         (%%vector-select <? v2 k2 0 n)))))))))))

;;; Like %%vector-select, but returns two values:
;;;
;;;     (vector-ref (vector-sort <? (vector-copy v start end)) k)
;;;     (vector-ref (vector-sort <? (vector-copy v start end)) (+ k 1))
;;;
;;; Returning two values is useful when finding the median of an even
;;; number of things.

(define (%%vector-select2 <? v k start end)
  (let ((size (- end start)))
    (cond ((= 2 size)
           (let ((a (vector-ref v start))
                 (b (vector-ref v (+ start 1))))
             (cond ((<? a b)
                    (values a b))
                   (else
                    (values b a)))))
          ((< size just-sort-it-threshold)
           (let ((v2 (r6rs-vector-sort <? (r7rs-vector-copy v start end))))
             (values (vector-ref v2 k)
                     (vector-ref v2 (+ k 1)))))
          (else
           (let* ((ip (random-integer size))
                  (pivot (vector-ref v (+ start ip))))
             (call-with-values
              (lambda () (count-smaller <? pivot v start end 0 0))
              (lambda (count count2)
                (cond ((= (+ k 1) count)
                       (values (%%vector-select <? v k start end)
                               pivot))
                      ((< k count)
                       (let* ((n count)
                              (v2 (make-vector n)))
                         (copy-smaller! <? pivot v2 0 v start end)
                         (%%vector-select2 <? v2 k 0 n)))
                      ((< k (+ count count2))
                       (values pivot
                               (if (< (+ k 1) (+ count count2))
                                   pivot
                                   (%%vector-select <? v (+ k 1) start end))))
                      (else
                       (let* ((n (- size count count2))
                              (v2 (make-vector n))
                              (k2 (- k count count2)))
                         (copy-bigger! <? pivot v2 0 v start end)
                         (%%vector-select2 <? v2 k2 0 n)))))))))))

;;; Counts how many elements within the range are less than the pivot
;;; and how many are equal to the pivot, returning both of those counts.

(define (count-smaller <? pivot v i end count count2)
  (cond ((= i end)
         (values count count2))
        ((<? (vector-ref v i) pivot)
         (count-smaller <? pivot v (+ i 1) end (+ count 1) count2))
        ((<? pivot (vector-ref v i))
         (count-smaller <? pivot v (+ i 1) end count count2))
        (else
         (count-smaller <? pivot v (+ i 1) end count (+ count2 1)))))

;;; Like vector-copy! but copies an element only if it is less than the pivot.
;;; The destination vector must be large enough.

(define (copy-smaller! <? pivot dst at src start end)
  (cond ((= start end) dst)
        ((<? (vector-ref src start) pivot)
         (vector-set! dst at (vector-ref src start))
         (copy-smaller! <? pivot dst (+ at 1) src (+ start 1) end))
        (else
         (copy-smaller! <? pivot dst at src (+ start 1) end))))

;;; Like copy-smaller! but copies only elements that are greater than the pivot.

(define (copy-bigger! <? pivot dst at src start end)
  (cond ((= start end) dst)
        ((<? pivot (vector-ref src start))
         (vector-set! dst at (vector-ref src start))
         (copy-bigger! <? pivot dst (+ at 1) src (+ start 1) end))
        (else
         (copy-bigger! <? pivot dst at src (+ start 1) end))))

