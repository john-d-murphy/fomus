(defun notes ()
  (fms:note :time 0 :dur 2 :pitch 70)
  (fms:note :time 2 :dur 2 :pitch 71)
  (fms:note :time 4 :dur 2 :pitch 72))

(fms:with-score
    (:filename *filename*
     :insts '((:id harpsichord :name "Harpsichord"
               :staves ((:clefs ((:instclef treble ; top staff
        			  :ledgers-up 3 :ledgers-down 2
        			  :octs-up 2 :octs-down 0)
        			 (:instclef bass
        			  :clef-preference 1/3
        			  :ledgers-up 2 :ledgers-down 3
        			  :octs-up 0 :octs-down 2)))
        		(:clefs ((:instclef bass ; bottom staff
        			  :ledgers-up 2 :ledgers-down 3
        			  :octs-up 0 :octs-down 2)
        			 (:instclef treble
        			  :clef-preference 1/3
        			  :ledgers-up 3 :ledgers-down 2
        			  :octs-up 2 :octs-down 0))))
               :min-pitch 29 :max-pitch 89))
     :parts '((:id prt :inst harpsichord)))
  (notes))
