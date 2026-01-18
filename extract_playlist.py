import re

input_file = r'C:\Users\Windows\Desktop\ProxyMedia\ListaVip.m3u8'
output_file = r'C:\Users\Windows\Desktop\ProxyMedia\ListaVip_Reduzida.m3u8'

# Categorias expandidas com mais canais
limites = {
    'GLOBO': 6,
    'SBT': 4,
    'RECORD': 4,
    'BAND': 4,
    'REDE TV': 3,
    'TV CULTURA': 3,
    'ESPN': 8,
    'SPORTV': 8,
    'PREMIERE': 12,
    'HBO MAX': 8,
    'HBO ': 6,
    'DISNEY+': 10,
    'TELECINE': 10,
    'CINE SKY': 10,
    'FILMES': 15,
    'INFANTIL': 15,
    'DISCOVERY': 8,
    'HISTORY': 4,
    'ANIMAL': 3,
    'NATIONAL': 4,
    'ESPORTES': 15,
    'NOTICIAS': 8,
    'COMBATE': 4,
    'UFC': 4,
    'TNT': 6,
    'SPACE': 4,
    'AXN': 4,
    'FX': 4,
    'WARNER': 6,
    'UNIVERSAL': 4,
    'COMEDY': 3,
    'STAR': 6,
    'AMC': 4,
    'SONY': 4,
    'PARAMOUNT': 6,
    'NETFLIX': 6,
    'PRIME': 6,
    'DAZN': 4,
    'CAZÉ': 4,
    'BAND NEWS': 3,
    'GLOBO NEWS': 3,
    'CNN': 3,
    'CARTOON': 4,
    'NICK': 6,
    'DISNEY CH': 6,
    'BOOMERANG': 3,
    'GLOOB': 4,
    'FOOD': 3,
    'TLC': 3,
    'LIFETIME': 3,
    'GNT': 4,
    'MULTISHOW': 4,
    'COMEDY': 3,
    'MUSIC': 4,
    'MTV': 4,
    'VH1': 3,
    'BIS': 3,
}

contagem = {k: 0 for k in limites}
resultado = ['#EXTM3U']

print('Processando playlist completa...')

with open(input_file, 'r', encoding='utf-8', errors='ignore') as f:
    linhas = f.readlines()

print(f'Total de linhas: {len(linhas)}')

i = 0
total_canais = 0
canais_adicionados = set()  # Evitar duplicatas

while i < len(linhas) - 1:
    linha = linhas[i].strip()
    
    if linha.startswith('#EXTINF'):
        url = linhas[i + 1].strip() if i + 1 < len(linhas) else ''
        
        # Priorizar FHD e HD
        is_fhd = 'FHD' in linha
        is_hd = 'HD' in linha and 'SD' not in linha
        is_qualidade = is_fhd or is_hd
        
        # Extrair nome do canal
        nome_match = re.search(r',(.+)$', linha)
        nome = nome_match.group(1).strip() if nome_match else ''
        
        if is_qualidade and url.startswith('http') and nome not in canais_adicionados:
            for cat, limite in limites.items():
                if cat.upper() in linha.upper() and contagem[cat] < limite:
                    resultado.append(linha)
                    resultado.append(url)
                    contagem[cat] += 1
                    total_canais += 1
                    canais_adicionados.add(nome)
                    break
        
        i += 2
    else:
        i += 1
    
    # Limite de 300 canais (~60KB)
    if total_canais >= 300:
        break

# Salvar
with open(output_file, 'w', encoding='utf-8') as f:
    f.write('\n'.join(resultado))

print(f'\n=== Playlist Reduzida Criada ===')
print(f'Total de canais: {total_canais}')
print(f'\nPor categoria:')
for cat, count in sorted(contagem.items(), key=lambda x: -x[1]):
    if count > 0:
        print(f'  {cat}: {count}')

import os
size = os.path.getsize(output_file)
print(f'\nTamanho do arquivo: {size/1024:.2f} KB')
print(f'Arquivo salvo em: {output_file}')
