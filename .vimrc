" on linux install gvim to have a clipboard support (gvim also installs vim program)
set expandtab
set shiftwidth=4
set softtabstop=4
syntax on
set smartcase
set ignorecase
set hlsearch
filetype indent on
set smartindent
inoremap <S-Tab> <C-V><Tab>
inoremap jj <Esc>
vmap <C-c> "+y
vmap <C-x> "+d
vmap <C-v> c<Esc>"+p
imap <C-v> <C-r><C-o>+
