\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{flashcards}[25/10/2019 flashcards class by William Strong]

\LoadClass{article}

\DeclareOption*{\PassOptionsToClass{\CurrentOption}{article}}
\ProcessOptions\relax

\usepackage{color}
\usepackage[paperwidth=74mm,paperheight=105mm,landscape]{geometry}

\newcommand{\answer}[1]{\highlight{#1}}
\newcommand{\back}[1]{\front{#1}}
\newcommand{\flashcard}[2]{
	\front{#1}
	\back{#2}
}
\newcommand{\front}[1]{
	\newpage

	\vspace*{\fill}
	\begin{center}
		#1
	\end{center}
	\vspace*{\fill}
}
\newcommand{\highlight}[1]{\textcolor{blue}{#1}}
\newcommand{\hint}[1]{\highlight{[#1]}}

\pagenumbering{gobble}
