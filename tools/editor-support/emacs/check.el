;;; check.el --- assert that ceps-mode's faces land  -*- lexical-binding: t; -*-

;; Run from the repository root:
;;
;;     emacs --batch -l tools/editor-support/emacs/check.el
;;
;; Exits non-zero if any expected face is missing. With -v it dumps every
;; distinct token and the faces it carries.

;;; Code:

(defvar ceps-check-dir
  (file-name-directory (or load-file-name buffer-file-name default-directory)))

(add-to-list 'load-path ceps-check-dir)
(require 'ceps-mode)

(defconst ceps-check-expectations
  '(("m"                     . ceps-si-unit-face)
    ("s"                     . ceps-si-unit-face)
    ("kg"                    . ceps-si-unit-face)
    ("mol"                   . ceps-si-unit-face)
    ("cd"                    . ceps-si-unit-face)
    ("A"                     . font-lock-variable-name-face)
    ("K"                     . font-lock-variable-name-face)
    ("g"                     . font-lock-variable-name-face)
    ("kind"                  . font-lock-keyword-face)
    ("Event"                 . font-lock-type-face)
    ("val"                   . font-lock-keyword-face)
    ("static_for"            . font-lock-keyword-face)
    ("ldi32"                 . font-lock-function-name-face)
    ("addi32"                . font-lock-function-name-face)
    ("OblectamentaDataLabel" . font-lock-type-face)
    ("sm"                    . font-lock-builtin-face)
    ("t"                     . font-lock-builtin-face)
    ("100"                   . font-lock-constant-face)
    ("3.14"                  . font-lock-constant-face))
  "Each entry is a token and a face it must carry somewhere in the fixture.")

(defun ceps-check--faces-at (pos)
  "Every face in effect at POS, as a list."
  (let ((f (or (get-text-property pos 'face)
               (get-text-property pos 'font-lock-face))))
    (cond ((null f) nil)
          ((listp f) f)
          (t (list f)))))

(defun ceps-check--faces-of (word)
  "Every face seen on a standalone occurrence of WORD in the current buffer."
  (let ((re (concat "\\_<" (regexp-quote word) "\\_>"))
        (acc '()))
    (save-excursion
      (goto-char (point-min))
      (while (re-search-forward re nil t)
        (dolist (f (ceps-check--faces-at (match-beginning 0)))
          (cl-pushnew f acc))))
    acc))

(let* ((sample (expand-file-name "../sample.ceps" ceps-check-dir))
       (verbose (member "-v" command-line-args))
       (failures 0))
  (unless (file-exists-p sample)
    (message "check.el: missing fixture %s" sample)
    (kill-emacs 2))
  (with-temp-buffer
    (insert-file-contents sample)
    (ceps-mode)
    (font-lock-mode 1)
    (font-lock-ensure (point-min) (point-max))

    (when verbose
      (save-excursion
        (goto-char (point-min))
        (while (re-search-forward "\\_<[A-Za-z_0-9.]+\\_>" nil t)
          (message "%-26s %s"
                   (match-string 0)
                   (or (ceps-check--faces-at (match-beginning 0)) "-")))))

    (dolist (expectation ceps-check-expectations)
      (let* ((word (car expectation))
             (want (cdr expectation))
             (got (ceps-check--faces-of word)))
        (unless (memq want got)
          (message "FAIL %-24s expected %s, got %s"
                   word want (or got "(token never seen)"))
          (setq failures (1+ failures))))))

  (if (> failures 0)
      (progn
        (message "check.el: %d expectation(s) failed" failures)
        (kill-emacs 1))
    (message "check.el: %d expectations hold"
             (length ceps-check-expectations))))

;;; check.el ends here
