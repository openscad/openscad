;;; scad-mode.el --- A major mode for editing OpenSCAD code

;; Author:     Len Trigg, Łukasz Stelmach
;; Maintainer: Len Trigg <lenbok@gmail.com>
;; Created:    March 2010
;; Modified:   28 Mar 2015
;; Keywords:   languages
;; URL:        https://raw.github.com/openscad/openscad/master/contrib/scad-mode.el
;; Version:    91.0

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
;; This is a major-mode to implement the SCAD constructs and
;; font-locking for OpenSCAD
;;
;; If installing manually, insert the following into your emacs startup:
;;
;; (autoload 'scad-mode "scad-mode" "A major mode for editing OpenSCAD code." t)
;; (add-to-list 'auto-mode-alist '("\\.scad$" . scad-mode))
;;
;; or
;;
;; install from marmalade: http://marmalade-repo.org/
;; M-x install-package <ENTER> scad-mode <ENTER>


;;; To Do:
;; - Support for background/debug/root/disable modifiers
;; - Font lock of non-built-in function calls

;;; Code:

;;;###autoload
(add-to-list 'auto-mode-alist '("\\.scad$" . scad-mode))

(require 'cc-mode)

(defcustom scad-command
  '"openscad"
  "Path to openscad executable"
  :type 'string
  )

(defcustom scad-keywords
  '("return" "true" "false")
  "SCAD keywords."
  :type 'list
  :group 'scad-font-lock)

(defcustom scad-functions
  '("cos" "acos" "sin" "asin" "tan" "atan" "atan2"                      ;;func.cc
    "abs" "sign" "rands" "min" "max" 
    "round" "ceil" "floor" 
    "pow" "sqrt" "exp" "log" "ln"
    "str" 
    "lookup" "version" "version_num" "len" "search"
    "dxf_dim" "dxf_cross"                                               ;;dxfdim.cc
    "norm" "cross"                                                      ;;2014.03
    "concat" "chr"                                                      ;;2015.03
    )
  "SCAD functions."
  :type 'list
  :group 'scad-font-lock)

(defcustom scad-modules
  '("children" "echo" "for" "intersection_for" "if" "else"              ;;control.cc
    "cube" "sphere" "cylinder" "polyhedron" "square" "circle" "polygon" ;;primitives.cc
    "scale" "rotate" "translate" "mirror" "multmatrix"                  ;;transform.cc
    "union" "difference" "intersection"                                 ;;csgops.cc
    "render"                                                            ;;render.cc
    "color"                                                             ;;color.cc
    "surface"                                                           ;;surface.cc
    "linear_extrude"                                                    ;;linearextrude.cc
    "rotate_extrude"                                                    ;;rotateextrude.cc
    "import"                                                            ;;import.cc
    "group"                                                             ;;builtin.cc
    "projection"                                                        ;;projection.cc
    "minkowski" "glide" "subdiv" "hull" "resize"                        ;;cgaladv.cc
    "parent_module"                                                     ;;2014.03
    "let" "offset" "text"                                               ;;2015.03
    )
  "SCAD modules."
  :type 'list
  :group 'scad-font-lock)

(defcustom scad-deprecated
  '("child" "assign" "dxf_linear_extrude" "dxf_rotate_extrude" 
    "import_stl" "import_off" "import_dxf")
  "SCAD deprecated modules and functions."
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
    (define-key map [(control c) (control o)] 'scad-open-current-buffer)
    (define-key map [return] 'newline-and-indent)
    ;;(define-key map [(control c) (control s)] 'c-show-syntactic-information) ;; Debugging info
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

    st)
  "Syntax table for `scad-mode'.")

(defvar scad-keywords-regexp (regexp-opt scad-keywords 'words))
(defvar scad-modules-regexp (regexp-opt scad-modules 'words))
(defvar scad-functions-regexp (regexp-opt scad-functions 'words))
(defvar scad-deprecated-regexp (regexp-opt scad-deprecated 'words))
(defvar scad-operators-regexp (regexp-opt scad-operators))

(defvar scad-font-lock-keywords
  `(
    ("\\(module\\|function\\)[ \t]+\\(\\sw+\\)" (1 'font-lock-keyword-face nil) (2 'font-lock-function-name-face nil t))
    ("\\(use\\|include\\)[ \t]*<\\([^>]+\\)>" (1 'font-lock-preprocessor-face nil) (2 'font-lock-type-face nil t))
    ("<\\(\\sw+\\)>" (1 'font-lock-builtin-face nil))
    ("$\\(\\sw+\\)" (1 'font-lock-builtin-face nil))
    (,scad-keywords-regexp . font-lock-keyword-face)
    (,scad-modules-regexp .  font-lock-builtin-face)
    (,scad-functions-regexp .  font-lock-function-name-face)
    (,scad-deprecated-regexp .  font-lock-warning-face)
    ;(,scad-operators-regexp .  font-lock-operator-face) ;; This actually looks pretty ugly
    ;("\\(\\<\\S +\\>\\)\\s *(" 1 font-lock-function-name-face t) ;; Seems to override other stuff (e.g. in comments and builtins)
    )
  "Keyword highlighting specification for `scad-mode'.")
(defconst scad-font-lock-keywords-1 scad-font-lock-keywords)
(defconst scad-font-lock-keywords-2 scad-font-lock-keywords)
(defconst scad-font-lock-keywords-3 scad-font-lock-keywords)

(defvar scad-indent-style nil
  "The style of indentation for scad-mode. Defaults to \"k&r\" if
  nil. If you want to set the style with file local variables use
  the `c-file-style' variable")

(put 'scad-mode 'c-mode-prefix "scad-")
;;;###autoload
(define-derived-mode scad-mode prog-mode "SCAD"
  "Major mode for editing OpenSCAD code.

To see what version of CC Mode you are running, enter `\\[c-version]'.

The hook `c-mode-common-hook' is run with no args at mode
initialization, then `scad-mode-hook'.

Key bindings:
\\{scad-mode-map}"
  (c-initialize-cc-mode)
  ;; (setq local-abbrev-table scad-mode-abbrev-table
  ;; 	abbrev-mode t)
  (use-local-map scad-mode-map)
  (c-set-offset (quote cpp-macro) 0 nil)
  (c-basic-common-init 'scad-mode (or scad-indent-style "k&r"))
  (c-font-lock-init)
  (c-run-mode-hooks 'c-mode-common-hook 'scad-mode-hook)
  (c-update-modeline))

;; From: http://stackoverflow.com/questions/14520073/add-words-for-dynamic-expansion-to-emacs-mode
(defun scad-prime-dabbrev ()
  "Makes a hidden scad-mode buffer containing all the scad keywords, so dabbrev expansion just works."
  (unless (get-buffer " *scad words*")
    (with-current-buffer (get-buffer-create " *scad words*")
      (scad-mode)
      (insert "module function use include")  ; Explicitly add these -- they're not in the below vars
      (insert (mapconcat 'identity (append scad-keywords scad-functions scad-modules scad-deprecated) " ")))))
(add-hook 'scad-mode-hook 'scad-prime-dabbrev)

(defun scad-open-current-buffer ()
  (interactive)
  (call-process scad-command nil 0 nil (buffer-file-name)))

(provide 'scad-mode)
;;; scad-mode.el ends here
