;;; Vinjector Lisp Edition v2.6-PRO (Auto-Scanner)
;;; Ported & Enhanced for Termux

(defpackage :vinjector
  (:use :cl :uiop))

(in-package :vinjector)

;; Colors
(defparameter +r+ "\u001b[31m") (defparameter +g+ "\u001b[32m")
(defparameter +y+ "\u001b[33m") (defparameter +c+ "\u001b[36m")
(defparameter +w+ "\u001b[37m") (defparameter +bold+ "\u001b[1m")
(defparameter +reset+ "\u001b[0m")

(defstruct config
  video-path gif-path icon-path (duration 10)
  (output-folder "output_v")
  (delete-source nil))

(defun log-msg (tag msg color)
  (format t "~A[~A~A~A~A] ~A~A~%" +w+ color +bold+ tag +reset+ +w+ msg +reset+))

(defun get-video-duration (path)
  (let ((out (uiop:run-program 
              (list "ffprobe" "-v" "error" "-show_entries" "format=duration" 
                    "-of" "default=noprint_wrappers=1:nokey=1" (namestring path))
              :output :string :ignore-error-status t)))
    (if (and out (not (string= out "")))
        (or (ignore-errors (read-from-string out)) 0)
        0)))

(defun scan-videos ()
  "Mencari file video di direktori saat ini."
  (let ((extensions '("*.mp4" "*.mkv" "*.mov")))
    (loop for ext in extensions
          append (directory ext))))

(defun process-file (file-path cfg)
  (let* ((filename (file-namestring file-path))
         (total-dur (get-video-duration file-path))
         (duration (config-duration cfg)))
    
    (if (<= total-dur 0)
        (progn (log-msg "SKIP" (format nil "Invalid video: ~A" filename) +r+) (return-from process-file))
        (log-msg "WORK" (format nil "Processing: ~A (~As)" filename (floor total-dur)) +c+))

    (ensure-directories-exist (format nil "~A/" (config-output-folder cfg)))

    ;; Split
    (uiop:run-program 
     (list "ffmpeg" "-v" "error" "-i" (namestring file-path)
           "-f" "segment" "-segment_time" (write-to-string duration)
           "-c" "copy" "-reset_timestamps" "1" "temp_v_%03d.mp4" "-y"))

    (let ((frags (sort (directory "temp_v_*.mp4") #'string-lessp :key #'namestring)))
      (loop for frag in frags for i from 1 do
        (let ((out-name (format nil "~A/~A_part~A.mp4" (config-output-folder cfg) (pathname-name file-path) i))
              (v-filter "[0:v]scale=-2:400,pad=720:480:(720-iw)/2:40,setsar=1[m];[1:v]scale=720:40[t];[2:v]scale=720:40[b];[3:v]scale=100:-1,format=rgba,colorchannelmixer=aa=0.8[l];[m][t]overlay=0:0:shortest=1[v1];[v1][b]overlay=0:H-h:shortest=1[v2];[v2][l]overlay=20:50"))
          
          (format t "~C~A[INJECTING ~A/~A]~A" #\Return +y+ i (length frags) +reset+)
          (finish-output)

          (uiop:run-program
           (list "ffmpeg" "-v" "error" "-i" (namestring frag)
                 "-ignore_loop" "0" "-i" (config-gif-path cfg)
                 "-ignore_loop" "0" "-i" (config-gif-path cfg)
                 "-i" (config-icon-path cfg)
                 "-filter_complex" v-filter "-map" "0:a?" "-c:v" "libx264"
                 "-b:v" "800k" "-preset" "ultrafast" "-shortest" out-name "-y"))
          (delete-file frag)))
      
      (format t "~%~A[DONE] ~A finished.~%~%" +g+ filename)
      (when (config-delete-source cfg) (delete-file file-path)))))

(defun main ()
  (let ((args uiop:*command-line-arguments*)
        (cfg (make-config :gif-path "overlay.gif" :icon-path "logo.png")))
    
    ;; Parsing argumen sederhana
    (loop for (key val) on args by #'cddr do
      (cond
        ((string= key "-f") (setf (config-gif-path cfg) val))
        ((string= key "-i") (setf (config-icon-path cfg) val))
        ((string= key "-d") (setf (config-duration cfg) (parse-integer val)))
        ((string= key "-x") (setf (config-delete-source cfg) t))))

    (format t "~A~AVinjector v2.6 PRO - Auto-Scanner~A~%~%" +bold+ +g+ +reset+)

    (let ((targets (scan-videos)))
      (if (null targets)
          (log-msg "VOID" "No video files found (*.mp4, *.mkv)!" +r+)
          (progn
            (log-msg "SCAN" (format nil "Found ~A videos." (length targets)) +g+)
            (dolist (video targets)
              (process-file video cfg))
            (log-msg "EXIT" "All tasks completed." +bold+))))))

(main)
