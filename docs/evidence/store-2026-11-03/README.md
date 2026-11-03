# Evidências — Microsoft Store, 03/11/2026

Os resumos WACK x64/x86 preservam o hash dos relatórios XML originais locais,
saída do processo, execução completa e resultado individual de 24 testes por
arquitetura. Os XML originais ficam em `build/store/wack-final-{x64,x86}/report.xml`;
não são publicados com metadados de segurança da máquina.

`summary.json` reúne identidade, hashes, testes, instalação/remoção e as quatro
capturas visuais revisadas. Os arquivos de áudio são sintéticos, originais e
ficam somente em build. As capturas finais estão em
`packaging/store/screenshots` e não contêm pastas pessoais nem diálogos.

Limites: testes executados no Windows 11 build 26200. O WACK e os testes locais
não equivalem à certificação da Store. O pacote de instalação de teste usa uma
identidade separada; seus EXEs são idênticos aos EXEs do pacote oficial por SHA-256.
O portal contém somente Submission 1 em rascunho. Nenhum arquivo foi enviado à Store.
