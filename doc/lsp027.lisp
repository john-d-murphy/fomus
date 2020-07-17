(defun notes (maxtim parts perc pnotes)
  (loop
     for tim = 0 then (+ tim (if (< (random 1.0) 0.5) 1/2 1))
     while (< tim maxtim)
     if (< (random 1.0) 0.666)
     do (fms:note :part (nth (random (length parts)) parts) ; woodwind note
                  :time tim
                  :dur 1/2
                  :pitch (+ 72 (random 7)))
     else do (fms:note :part perc ; percussion note
                       :time tim
                       :dur 1/2
                       :pitch (nth (random (length pnotes)) pnotes))))

(fms:with-score
    (:filename *filename*
     :sets '(:layout "orchestra")
     :percinsts '((:id "wb1" :template "low-woodblock" :perc-note 57 :perc-name "woodblocks")
                  (:id "wb2" :template "high-woodblock" :perc-note 64 :perc-name "woodblocks"))
     :insts '((:id "prcdef" :template "percussion" :percinsts ("wb1" "wb2")))
     :parts '((:id "cl1" :name "Bf Clarinet 1" :abbr "cl 1" :inst "bflat-clarinet" :transpose-part nil)
              (:id "cl2" :name "Bf Clarinet 2" :abbr "cl 2" :inst "bflat-clarinet" :transpose-part nil)
              (:id "ob" :name "Oboe" :abbr "ob" :inst "oboe")
              (:id "prc" :name "Percussion" :abbr "perc" :inst "prcdef")))
  (notes 16 '("cl1" "cl2" "ob") "prc" '("wb1" "wb2")))
