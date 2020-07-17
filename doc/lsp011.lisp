(defparameter *parts*
  '((:id "fl1" :name "Flute 1" :inst "flute")
    (:id "fl2" :name "Flute 2" :inst "flute")
    (:id "cl2" :name "Clarinet 1" :inst "bflat-clarinet")
    (:id "vln1" :name "Violin 1" :inst "violin")
    (:id "vln2" :name "Violin 2" :inst "violin")
    (:id "vc1" :name "Cello 1" :inst "cello")
    (:id "vc2" :name "Cello 2" :inst "cello")
    (:id "tba" :name "Tuba" :inst "tuba")))

(defun notes (p)
  (loop
     for tim from 0
     for n in '(60 62 64)
     do (fms:note :part p :time tim :dur 1 :pitch n)))

(defun parts (ps)
  (loop
     for p in ps
     do (notes p)))

(fms:with-score
    (:filename *filename*
     :sets '(:layout "orchestra")
     :parts *parts*)
  (parts '("fl1" "fl2" "cl2" "vln1" "vln2" "vc1" "vc2" "tba")))
