(defparameter *insts*
  '((:id "piano" :template "piano"
     ;; a list of staff objects, each containing a list of clef objects
     ;; the first clef object in the list is the default for that staff
     :staves ((:clefs ((:instclef "treble" ; defines a treble clef
                        :ledgers-down 2 ; approx. 2 ledger lines allowed below staff
                        :octs-down 0) ; no 8vb octave signs allowed
                       (:instclef "bass" ; defines a bass clef
                        :clef-preference 1/2 ; clef is 1/2 as likely to be chosen
                        :ledgers-up 2 ; approx. 2 ledger lines allowed above staff
                        :octs-up 0))) ; no 8va octave signs allowed
              (:clefs ((:instclef "treble" ; clef choices for middle staff...
                        :ledgers-down 2
                        :octs-down 0 :octs-up 0)
                       (:instclef "bass"
                        :ledgers-up 2
                        :octs-down 0 :octs-up 0)))
              (:clefs ((:instclef "bass" ; bass clef is the default for bottom staff
                        :ledgers-up 2
                        :octs-up 0)
                       (:instclef "treble"
                        :clef-preference 1/2
                        :ledgers-down 2
                        :octs-down 0)))))))

(defun notes (n)
  (loop
     repeat n
     for tim from 0
     do (fms:note :part "prt" :time tim :dur 1 :pitch (+ 33 (random 64)))))

(fms:with-score
    (:filename *filename*
     :insts *insts* ; instruments are always defined first in a `with-score' macro
     :parts '((:id "prt" :inst "piano")))
  (notes 20))
