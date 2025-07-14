#!/bin/bash

# Script de teste para o simulador FAT16
# Este script demonstra o funcionamento básico do sistema

echo "Teste do Simulador FAT16"
echo

# Verifica se o executável existe
if [ ! -f "./fat16" ]; then
    echo "Executável não encontrado. Compilando..."
    make
    if [ $? -ne 0 ]; then
        echo "Erro na compilação!"
        exit 1
    fi
fi

echo "Iniciando teste automatizado..."
echo

# Cria arquivo de comandos de teste
cat > test_commands.txt << 'EOF'
init
ls
mkdir /documentos
mkdir /imagens
mkdir /backup
ls /
create /documentos/relatorio.txt
create /documentos/notas.txt
create /imagens/foto1.jpg
write "Este é um relatório de teste do sistema FAT16" /documentos/relatorio.txt
write "Notas importantes do projeto" /documentos/notas.txt
write "Dados binários simulados da imagem" /imagens/foto1.jpg
ls /documentos
read /documentos/relatorio.txt
append " - Adicionando mais conteúdo" /documentos/relatorio.txt
read /documentos/relatorio.txt
mkdir /documentos/projetos
create /documentos/projetos/projeto1.txt
write "Conteúdo do projeto 1" /documentos/projetos/projeto1.txt
ls /documentos/projetos
read /documentos/projetos/projeto1.txt
unlink /documentos/projetos/projeto1.txt
unlink /documentos/projetos
ls /documentos
create /backup/backup.txt
write "Arquivo de backup" /backup/backup.txt
ls /backup
unlink /backup/backup.txt
unlink /backup
ls /
exit
EOF

echo "Executando comandos de teste..."
echo "================================"

# Executa o simulador com os comandos de teste
./fat16 < test_commands.txt

echo "================================"
echo "Teste concluído!"
echo

# Verifica se o arquivo de partição foi criado
if [ -f "fat.part" ]; then
    echo "✓ Arquivo de partição criado com sucesso"
    echo "  Tamanho: $(ls -lh fat.part | awk '{print $5}')"
else
    echo "✗ Erro: Arquivo de partição não foi criado"
fi

# Limpa arquivos temporários
rm -f test_commands.txt

echo
echo "Para testar manualmente, execute:"
echo "./bin/fat16_simulator"
echo
echo "Comandos úteis:"
echo "  init          - Inicializar sistema"
echo "  load          - Carregar sistema existente"
echo "  ls /          - Listar diretório raiz"
echo "  mkdir /teste  - Criar diretório"
echo "  create /arquivo.txt - Criar arquivo"
echo "  write \"texto\" /arquivo.txt - Escrever no arquivo"
echo "  read /arquivo.txt - Ler arquivo"
echo "  help          - Mostrar ajuda completa"
echo "  exit          - Sair"