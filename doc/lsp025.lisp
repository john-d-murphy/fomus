;; if there is an existing score object, it is destroyed
(fms:free)
;; this destroys an existing score object and creates a new one
(fms:new)
;; (both `free' and `new' effectively accomplish the exact same thing--the only
;; difference being that after `new' there is an empty score object sitting in memory)

;; `filename' is set using the `setting' function
(fms:setting :filename *filename*) ; a score object is automatically created here if one doesn't exist

(loop
   for o from 0 below 20 by 1/2
   do (fms:note :time o :dur 1/2 :pitch (+ 65 (random 15))))

;; the `run' function processes the score
(fms:run)

(fms:free) ; destroy the score and free up memory
