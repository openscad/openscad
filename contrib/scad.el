;;; scad.el --- SCAD mode derived mode

;; Author:     Len Trigg
;; Maintainer: Len Trigg <lenbok@gmail.com>
;; Created:    March 2010
;; Modified:   March 2010
;; Version:    $Revision: 88 $

;; This program is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 2 of the License, or
;; (at your option) any later version.
;; 
;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;; 
;; You should have received a copy of the GNU General Public License
;; along with this program; see the file COPYING.  If not, write to
;; the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
;; Boston, MA 02111-1307, USA.

;;; Commentary:
;;
;; This is a separate mode to implement the SCAD constructs and
;; font-locking.
;;
;; To use, insert the following into your emacs startup:
;;
;; (autoload 'scad-mode "scad" "Major mode for editing SCAD code." t)
;; (add-to-list 'auto-mode-alist '("\\.scad$" . scad-mode))

;;; To Do:
;; - Support for background/debug/root/disable modifiers
;; - Font lock of non-built-in function calls

;;; Autoload mode trigger
(add-to-list 'auto-mode-alist '("\\.scad$" . scad-mode))

(defcustom scad-keywords
  '("return" "true" "false")
  "SCAD keywords."
  :type 'list
  :group 'scad-font-lock)

(defcustom scad-functions
  '("cos" "acos" "sin" "asin" "tan" "atan" 
    "pow" "log" "ln"
    "abs" "min" "max" "sqrt" "round" "ceil" "floor" "lookup" 
    "str" 
    "dxf_dim" "dxf_cross"
    )
  "SCAD functions."
  :type 'list
  :group 'scad-font-lock)

(defcustom scad-modules
  '("for" "if" "assign" "intersection_for"
    "echo"
    "cube" "sphere" "cylinder" "polyhedron" 
    "scale" "rotate" "translate" "mirror" "multmatrix" "color"
    "union" "difference" "intersection" "render" "surface"
    "square" "circle" "polygon" "dxf_linear_extrude" "linear_extrude" "dxf_rotate_extrude" "rotate_extrude"
    "import_stl" "import_off" "import_dxf" "group"
    "projection" "minkowski" "glide" "subdiv" "child"
    )
  "SCAD modules."
  :type 'list
  :group 'scad-font-lock)

(defcustom scad-operators
  '("+" "-" "*" "/" "%" 
    "&&" "||" "!" 
    "<" "<=" "==" "!=" ">" ">="
    "?" ":" "=")
  "SCAD operators."
  :type 'list
  :group 'scad-font-lock)

(defvar scad-mode-map
  (let ((map (make-sparse-keymap)))
    (define-key map "\t" 'scad-indent-line)
    (define-key map [return] 'newline-and-indent) 
    map)
  "Keymap for `scad-mode'.")

(defvar scad-mode-syntax-table
  (let ((st (make-syntax-table)))
    ;; support comment style: “// ...” 
    ;; support comment style: “/* ... */” 
    (modify-syntax-entry ?\/ ". 124b" st)
    (modify-syntax-entry ?\n "> b" st)
    (modify-syntax-entry ?* ". 23" st)

    ;; Extra punctuation
    (modify-syntax-entry ?+  "." st)
    (modify-syntax-entry ?-  "." st)
    (modify-syntax-entry ?%  "." st)
    (modify-syntax-entry ?<  "." st)
    (modify-syntax-entry ?>  "." st)
    (modify-syntax-entry ?&  "." st)
    (modify-syntax-entry ?:  "." st)
    (modify-syntax-entry ?|  "." st)
    (modify-syntax-entry ?=  "." st)
    (modify-syntax-entry ?\;  "." st)

    ;; _ allowed in word (alternatively "_" as symbol constituent?)
    (modify-syntax-entry ?_  "w" st)

    st)
  "Syntax table for `scad-mode'.")

(defvar scad-keywords-regexp (regexp-opt scad-keywords 'words))
(defvar scad-modules-regexp (regexp-opt scad-modules 'words))
(defvar scad-functions-regexp (regexp-opt scad-functions 'words))
(defvar scad-operators-regexp (regexp-opt scad-operators))

(defvar scad-font-lock-keywords
  `(
    ("\\(module\\|function\\)[ \t]+\\(\\sw+\\)" (1 'font-lock-keyword-face nil) (2 'font-lock-function-name-face nil t))
    ("<\\(\\sw+\\)>" (1 'font-lock-builtin-face nil))
    ("$\\(\\sw+\\)" (1 'font-lock-builtin-face nil))
    (,scad-keywords-regexp . font-lock-keyword-face)
    (,scad-modules-regexp .  font-lock-builtin-face)
    (,scad-functions-regexp .  font-lock-function-name-face)
    ;(,scad-operators-regexp .  font-lock-operator-face) ;; This actually looks pretty ugly
    ;("\\(\\<\\S +\\>\\)\\s *(" 1 font-lock-function-name-face t) ;; Seems to override other stuff (e.g. in comments and builtins)
    )
  "Keyword highlighting specification for `scad-mode'.")

;(defvar scad-imenu-generic-expression ...)
;(defvar scad-outline-regexp ...)

;;;###autoload
(define-derived-mode scad-mode fundamental-mode "SCAD"
  "A major mode for editing SCAD files."
  :syntax-table scad-mode-syntax-table
  (set (make-local-variable 'font-lock-defaults) '(scad-font-lock-keywords))
  (set (make-local-variable 'indent-line-function) 'scad-indent-line)
  ;(set (make-local-variable 'imenu-generic-expression) scad-imenu-generic-expression)
  ;(set (make-local-variable 'outline-regexp) scad-outline-regexp)
  )


;;; Indentation, based on http://www.emacswiki.org/emacs/download/actionscript-mode-haas-7.0.el

(defun scad-indent-line ()
  "Indent current line of SCAD code."
  (interactive)
  (let ((savep (> (current-column) (current-indentation)))
        (indent (condition-case nil (max (scad-calculate-indentation) 0)
                  (error 0))))
    (if savep
        (save-excursion (indent-line-to indent))
      (indent-line-to indent))))

(defun scad-calculate-indentation ()
  "Return the column to which the current line should be indented."
  (save-excursion
    (scad-maybe-skip-leading-close-delim)
    (let ((pos (point)))
      (beginning-of-line)
      (if (not (search-backward-regexp "[^\n\t\r ]" 1 0))
          0
        (progn
          (scad-maybe-skip-leading-close-delim)
          (+ (current-indentation) (* standard-indent (scad-count-scope-depth (point) pos))))))))

(defun scad-maybe-skip-leading-close-delim ()
  (beginning-of-line)
  (forward-to-indentation 0)
  (if (looking-at "\\s)")
      (forward-char)
    (beginning-of-line)))

(defun scad-face-at-point (pos)
  "Return face descriptor for char at point."
  (plist-get (text-properties-at pos) 'face))

(defun scad-count-scope-depth (rstart rend)
  "Return difference between open and close scope delimeters."
  (save-excursion
    (goto-char rstart)
    (let ((open-count 0)
          (close-count 0)
          opoint)
      (while (and (< (point) rend)
                  (progn (setq opoint (point))
                         (re-search-forward "\\s)\\|\\s(" rend t)))
        (if (= opoint (point))
            (forward-char 1)
          (cond
           ;; Don't count if in string or comment.
           ((scad-face-at-point (- (point) 1)))
           ((looking-back "\\s)")
            (incf close-count))
           ((looking-back "\\s(")
            (incf open-count))
           )))
      (- open-count close-count))))


(provide 'scad)
;;; scad.el ends here
